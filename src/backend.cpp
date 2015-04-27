/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2013 Joshua C. Klontz                                           *
 *                                                                           *
 * Licensed under the Apache License, Version 2.0 (the "License");           *
 * you may not use this file except in compliance with the License.          *
 * You may obtain a copy of the License at                                   *
 *                                                                           *
 *     http://www.apache.org/licenses/LICENSE-2.0                            *
 *                                                                           *
 * Unless required by applicable law or agreed to in writing, software       *
 * distributed under the License is distributed on an "AS IS" BASIS,         *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
 * See the License for the specific language governing permissions and       *
 * limitations under the License.                                            *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifdef _MSC_VER
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <llvm/ADT/Hashing.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/AssumptionCache.h>
#include <llvm/Analysis/CodeMetrics.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/ConstantRange.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/ManagedStatic.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Target/TargetSubtargetInfo.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Vectorize.h>
#include <cstdarg>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>
#include <thread>

#include "likely/backend.h"

using namespace llvm;
using namespace std;

//! [likely_default_settings implementation.]
likely_settings likely_default_settings(likely_file_type file_type, bool verbose)
{
    likely_settings settings;
    if ((file_type == likely_file_ir) || (file_type == likely_file_bitcode)) {
        settings.opt_level = 2;
        settings.size_level = 2;
        settings.multicore = false;
        settings.unroll_loops = false;
        settings.vectorize_loops = false;
    } else {
        settings.opt_level = 3;
        settings.size_level = 0;
        settings.unroll_loops = true;
        settings.vectorize_loops = true;
    }
    if (file_type == likely_file_void)
        settings.multicore = thread::hardware_concurrency() > 1;
    else
        settings.multicore = false;
    settings.heterogeneous = false;
    settings.runtime_only = false;
    settings.verbose = verbose;
    return settings;
}
//! [likely_default_settings implementation.]

namespace {

static bool IsOne(Value *value)
{
    if (ConstantInt *const constantInt = dyn_cast<ConstantInt>(value))
        return constantInt->isOne();
    return false;
}

struct AssumptionSubstitution : public FunctionPass
{
    static char ID;
    AssumptionSubstitution() : FunctionPass(ID) {}

    void getAnalysisUsage(AnalysisUsage &analysisUsage) const override
    {
        analysisUsage.setPreservesCFG();
        analysisUsage.addPreserved<AssumptionCacheTracker>();
        analysisUsage.addRequired<AssumptionCacheTracker>();
    }

    bool runOnFunction(Function &F) override
    {
        AssumptionCache &AC = getAnalysis<AssumptionCacheTracker>().getAssumptionCache(F);

        SmallPtrSet<const Value*, 1> EphValues;
        CodeMetrics::collectEphemeralValues(&F, &AC, EphValues);

        bool modified = false;
        for (const auto &assumption : AC.assumptions())
            if (CallInst *const callInst = dyn_cast_or_null<CallInst>((Value*)assumption))
                if (CmpInst *const cmpInst = dyn_cast<CmpInst>(callInst->getOperand(0)))
                    if (cmpInst->getPredicate() == CmpInst::ICMP_EQ) {
                        Value *const find    = cmpInst->getOperand(0);
                        Value *const replace = cmpInst->getOperand(1);
                        for (User *const user : find->users())
                            if (!EphValues.count(user)) {
                                user->replaceUsesOfWith(find, replace);
                                modified = true;
                            }
                    }

        return modified;
    }
};
char AssumptionSubstitution::ID = 0;

static bool ExperimentalLoopCollapse = false;
/*
 * Collapse nested loops of the form:
 * for (int i=0; i<x; i++)
 *   for (int j=0; j<y; j++)
 *     f(i*y+j)
 *
 * To:
 * for (int i=0; i<x*y; i++)
 *   f(i)
 */
struct LoopCollapse : public LoopPass
{
    static char ID;
    LoopCollapse() : LoopPass(ID) {}

    static void getIncrementAndExitCriteria(Loop *const loop, ICmpInst *&postcondition, AddOperator *&increment, Value *&exitCriteria)
    {
        postcondition = NULL;
        increment = NULL;
        exitCriteria = NULL;
        Value *const CIV = loop->getCanonicalInductionVariable();
        BasicBlock *const loopLatch = loop->getLoopLatch();
        if (!CIV || !loopLatch)
            return;

        if (BranchInst *const branchInst = dyn_cast<BranchInst>(loopLatch->getTerminator())) {
            postcondition = dyn_cast<ICmpInst>(branchInst->getOperand(0));
            if (postcondition) {
                if (AddOperator *const addOperator = dyn_cast<AddOperator>(postcondition->getOperand(0)))
                    if ((IsOne(addOperator->getOperand(0)) && (addOperator->getOperand(1) == CIV)) ||
                        (IsOne(addOperator->getOperand(1)) && (addOperator->getOperand(0) == CIV)))
                        increment = addOperator;
                if (loop->isLoopInvariant(postcondition->getOperand(1)))
                    exitCriteria = postcondition->getOperand(1);
            }
        }
    }

    bool runOnLoop(Loop *loop, LPPassManager &) override
    {
        PHINode *const CIV = loop->getCanonicalInductionVariable();
        if (!CIV)
            return false;

        Loop *const parent = loop->getParentLoop();
        if (!parent)
            return false;

        PHINode *const parentCIV = parent->getCanonicalInductionVariable();
        if (!parentCIV)
            return false;

        if (CIV->getType() != parentCIV->getType())
            return false;

        ICmpInst *postcondition, *parentPostcondition;
        AddOperator *increment, *parentIncrement;
        Value *exitCriteria, *parentExitCriteria;
        getIncrementAndExitCriteria(loop, postcondition, increment, exitCriteria);
        getIncrementAndExitCriteria(parent, parentPostcondition, parentIncrement, parentExitCriteria);
        (void) parentExitCriteria;
        if (!increment || !exitCriteria || !parentIncrement)
            return false;

        // To be collapsible we must be able to pattern match all uses of parentCIV
        map<AddOperator*, Value*> outerAdds; // A mapping of outerAdd -> invariant
        for (User *const user : parentCIV->users()) {
            if (user == parentIncrement)
                continue;

            // We can only replace uses of the form `(parentCIV + invariant) * exitCriteria + CIV`
            // Check `parentCIV + invariant`
            AddOperator *const innerAdd = dyn_cast<AddOperator>(user);
            if (!innerAdd)
                return false;

            Value *invariant = NULL;
            if      (innerAdd->getOperand(0) == parentCIV) invariant = innerAdd->getOperand(1);
            else if (innerAdd->getOperand(1) == parentCIV) invariant = innerAdd->getOperand(0);
            else    return false;
            if (!parent->isLoopInvariant(invariant))
                return false;

            for (User *const user : innerAdd->users()) {
                // Check `innerAdd * exitCriteria`
                MulOperator *const mul = dyn_cast<MulOperator>(user);
                if (!mul)
                    return false;
                if (!(((mul->getOperand(0) == innerAdd) && (mul->getOperand(1) == exitCriteria)) ||
                      ((mul->getOperand(1) == innerAdd) && (mul->getOperand(0) == exitCriteria))))
                    return false;

                // Check `mul + CIV`
                for (User *const user : mul->users()) {
                    AddOperator *const outerAdd = dyn_cast<AddOperator>(user);
                    if (!outerAdd)
                        return false;
                    if (!(((outerAdd->getOperand(0) == mul) && (outerAdd->getOperand(1) == CIV)) ||
                          ((outerAdd->getOperand(1) == mul) && (outerAdd->getOperand(0) == CIV))))
                        return false;
                    outerAdds.insert(pair<AddOperator*,Value*>(outerAdd, invariant));
                }
            }
        }

        // To be collapsible we must have pattern matched all uses of CIV
        for (User *const user : CIV->users()) {
            if (user == increment)
                continue;

            if (outerAdds.find(dyn_cast_or_null<AddOperator>(user)) != outerAdds.end())
                continue;

            return false;
        }

        // Scale our exitCriteria by parentExitCriteria
        IRBuilder<> builder(loop->getLoopPreheader()->getTerminator());
        Value *const scaledExitCriteria = builder.CreateMul(parentExitCriteria, exitCriteria);
        postcondition->replaceUsesOfWith(exitCriteria, scaledExitCriteria);

        // Update the outer additions
        for (const pair<AddOperator*, Value*> &outerAdd : outerAdds) {
            // Since we wish for our new induction variable to be:
            // newCIV = parentCIV * exitCriteria + CIV
            //
            // It follows that we should re-write:
            // (invariant + parentCIV) * exitCriteria + CIV
            // as:
            // invariant * exitCriteria + newCIV
            builder.SetInsertPoint(cast<Instruction>(outerAdd.first));
            outerAdd.first->replaceAllUsesWith(builder.CreateAdd(builder.CreateMul(outerAdd.second, exitCriteria), CIV));
        }

        // Update our parent's range
        parentPostcondition->replaceUsesOfWith(parentExitCriteria, ConstantInt::get(parentExitCriteria->getType(), 1));

        return true;
    }
};
char LoopCollapse::ID = 0;

struct LikelyContext : public likely_settings
{
    legacy::PassManager *PM;
    AssumptionCacheTracker *ACT;
    LLVMContext context;

    LikelyContext(const likely_settings &settings)
        : likely_settings(settings), PM(new legacy::PassManager()), ACT(new AssumptionCacheTracker())
    {
        static TargetMachine *TM = NULL;
        if (!TM) {
            InitializeNativeTarget();
            InitializeNativeTargetAsmPrinter();
            InitializeNativeTargetAsmParser();

            PassRegistry &Registry = *PassRegistry::getPassRegistry();
            initializeCore(Registry);
            initializeScalarOpts(Registry);
            initializeVectorization(Registry);
            initializeIPO(Registry);
            initializeAnalysis(Registry);
            initializeIPA(Registry);
            initializeTransformUtils(Registry);
            initializeInstCombine(Registry);
            initializeTarget(Registry);

            TM = getTargetMachine(false);
        }

        PM->add(createVerifierPass());
        PM->add(ACT);
        PM->add(new TargetLibraryInfoWrapperPass(Triple(sys::getProcessTriple())));
        PM->add(createTargetTransformInfoWrapperPass(TM->getTargetIRAnalysis()));
        PassManagerBuilder builder;
        builder.OptLevel = opt_level;
        builder.SizeLevel = size_level;
        builder.DisableUnrollLoops = !unroll_loops;
        builder.LoopVectorize = vectorize_loops;
        builder.Inliner = createAlwaysInlinerPass();
        builder.addExtension(PassManagerBuilder::EP_Peephole, addPeephole);
        builder.addExtension(PassManagerBuilder::EP_LoopOptimizerEnd, addLoopOptimizerEnd);
        builder.populateModulePassManager(*PM);
        PM->add(createVerifierPass());
    }

    ~LikelyContext()
    {
        delete PM;
    }

    LikelyContext(const LikelyContext &) = delete;
    LikelyContext &operator=(const LikelyContext &) = delete;

    Type *toLLVM(likely_type likely, uint64_t elements = 0)
    {
        if (elements == 0) {
            auto const result = typeLUT.find(likely);
            if (result != typeLUT.end())
                return result->second;
        }

        Type *llvm;
        if (likely & likely_compound_pointer) {
            llvm = PointerType::getUnqual(toLLVM(likely_element_type(likely)));
        } else if (likely & likely_compound_struct) {
            const likely_const_mat name = likely_struct_name(likely);
            vector<likely_type> memberTypes(likely_struct_members(likely));
            likely_member_types(likely, memberTypes.data());
            vector<Type*> members;
            for (const likely_type memberType : memberTypes)
                members.push_back(toLLVM(memberType));
            llvm = members.empty() ? StructType::create(context, name->data)
                                   : StructType::create(members, name->data);
            likely_release_mat(name);
        } else if (likely & likely_multi_dimension) {
            stringstream name;
            const likely_const_mat str = likely_type_to_string(likely);
            name << str->data;
            likely_release_mat(str);
            if (elements != 0)
                name << "_" << elements;

            likely_type element = likely & likely_element;
            if (!(element & likely_depth))
                element |= likely_u8;
            llvm = PointerType::getUnqual(StructType::create(name.str(),
                                                             Type::getInt32Ty(context), // ref_count
                                                             Type::getInt32Ty(context), // type
                                                             Type::getInt32Ty(context), // channels
                                                             Type::getInt32Ty(context), // columns
                                                             Type::getInt32Ty(context), // rows
                                                             Type::getInt32Ty(context), // frames
                                                             ArrayType::get(toLLVM(element), elements), // data
                                                             NULL));
        } else if (likely == likely_void) {
            llvm = Type::getVoidTy(context);
        } else {
            const uint32_t bits = likely & likely_depth;
            if (likely & likely_floating) {
                if      (bits == 16) llvm = Type::getHalfTy(context);
                else if (bits == 32) llvm = Type::getFloatTy(context);
                else if (bits == 64) llvm = Type::getDoubleTy(context);
                else                 { llvm = NULL; likely_ensure(false, "invalid floating bits: %d", bits); }
            } else {
                llvm = Type::getIntNTy(context, bits);
            }
        }

        if (elements == 0)
            typeLUT[likely] = llvm;
        return llvm;
    }

    static TargetMachine *getTargetMachine(bool JIT)
    {
        static const Target *TheTarget = NULL;
        static TargetOptions TO;
        static mutex lock;
        lock_guard<mutex> locker(lock);

        if (TheTarget == NULL) {
            string error;
            TheTarget = TargetRegistry::lookupTarget(sys::getProcessTriple(), error);
            likely_ensure(TheTarget != NULL, "target lookup failed with error: %s", error.c_str());
            TO.LessPreciseFPMADOption = true;
            TO.UnsafeFPMath = true;
            TO.NoInfsFPMath = true;
            TO.NoNaNsFPMath = true;
            TO.AllowFPOpFusion = FPOpFusion::Fast;
        }

        string targetTriple = sys::getProcessTriple();
#ifdef _WIN32
        if (JIT)
            targetTriple += "-elf";
#endif // _WIN32

        TargetMachine *TM = TheTarget->createTargetMachine(targetTriple,
                                                           sys::getHostCPUName(),
                                                           "",
                                                           TO,
                                                           Reloc::Default,
                                                           JIT ? CodeModel::JITDefault : CodeModel::Default,
                                                           CodeGenOpt::Aggressive);
        likely_ensure(TM != NULL, "failed to create target machine");
        return TM;
    }

private:
    map<likely_type, Type*> typeLUT;

    static void addPeephole(const PassManagerBuilder &, legacy::PassManagerBase &PM)
    {
        PM.add(new AssumptionSubstitution());
    }

    static void addLoopOptimizerEnd(const PassManagerBuilder &, legacy::PassManagerBase &PM)
    {
        if (ExperimentalLoopCollapse)
            PM.add(new LoopCollapse());
    }
};

class JITFunctionCache : public ObjectCache
{
    map<hash_code, unique_ptr<MemoryBuffer>> cachedModules;
    map<const Module*, hash_code> currentModules;
    mutex lock;

    void notifyObjectCompiled(const Module *M, MemoryBufferRef Obj)
    {
        lock_guard<mutex> guard(lock);
        const auto currentModule = currentModules.find(M);
        const hash_code hash = currentModule->second;
        currentModules.erase(currentModule);
        cachedModules[hash] = MemoryBuffer::getMemBufferCopy(Obj.getBuffer());
    }

    unique_ptr<MemoryBuffer> getObject(const Module *M)
    {
        lock_guard<mutex> guard(lock);
        const auto currentModule = currentModules.find(M);
        const hash_code hash = currentModule->second;
        const auto cachedModule = cachedModules.find(hash);
        if (cachedModule != cachedModules.end()) {
            currentModules.erase(currentModule);
            return unique_ptr<MemoryBuffer>(MemoryBuffer::getMemBufferCopy(cachedModule->second->getBuffer()));
        }
        return unique_ptr<MemoryBuffer>();
    }

public:
    bool alert(const Module *M)
    {
        string str;
        raw_string_ostream ostream(str);
        M->print(ostream, NULL);
        ostream.flush();
        const hash_code hash = hash_value(str);

        lock_guard<mutex> guard(lock);
        const bool hit = (cachedModules.find(hash) != cachedModules.end());
        currentModules.insert(pair<const Module*, hash_code>(M, hash));
        return hit;
    }
};
static JITFunctionCache TheJITFunctionCache;

struct LikelyValue
{
    Value *value;
    likely_type type;

    LikelyValue(Value *value = NULL, likely_type type = likely_void)
        : value(value), type(type) {}

    operator Value*() const { return value; }
    operator likely_type() const { return type; }

    void dump() const
    {
        likely_const_mat m = likely_type_to_string(type);
        cerr << m->data << " ";
        value->dump();
        likely_release_mat(m);
    }

    static likely_type toLikely(Type *llvm)
    {
        if      (llvm->isIntegerTy()) return llvm->getIntegerBitWidth();
        else if (llvm->isHalfTy())    return likely_f16;
        else if (llvm->isFloatTy())   return likely_f32;
        else if (llvm->isDoubleTy())  return likely_f64;
        else {
            if (FunctionType *function = dyn_cast<FunctionType>(llvm)) {
                return toLikely(function->getReturnType());
            } else {
                if (Type *element = dyn_cast<PointerType>(llvm)->getElementType()) {
                    if (StructType *matrix = dyn_cast<StructType>(element)) {
                        return likely_type_from_string(matrix->getName().str().c_str(), NULL);
                    } else {
                        const likely_type type = toLikely(element);
                        return isa<FunctionType>(element) ? type : likely_pointer_type(type);
                    }
                } else {
                    return likely_void;
                }
            }
        }
    }

    static bool isMat(Type *type)
    {
        // This is safe because matricies are the only struct types created by the backend
        if (PointerType *ptr = dyn_cast<PointerType>(type))
            if (dyn_cast<StructType>(ptr->getElementType()))
                return true;
        return false;
    }
};

struct Builder;

} // namespace (anonymous)

struct Variant
{
    union {
        void *value = NULL;
        likely_const_ast ast;
        likely_const_env env;
        likely_const_mat mat;
    };
    likely_type type;

    Variant(void *value, likely_type type)
        : value(value), type(type)
    {
        // If it's actually a matrix, let's discover it's real type
        if (const likely_const_mat mat = *this)
            this->type = mat->type;
    }

    Variant()
        : Variant(NULL, likely_void) {}

    Variant(likely_const_mat mat)
        : Variant((void*) mat, mat ? mat->type : likely_type(likely_void)) {}

    Variant(likely_const_env env)
        : Variant((void*) env, envType()) {}

    ~Variant()
    {
        if (!value)
            return;

        if      (type == astType()) likely_release_ast(ast);
        else if (type == envType()) likely_release_env(env);
        else                        likely_release_mat(mat);
    }

    Variant(const Variant &other)
    {
        *this = other;
    }

    Variant &operator=(const Variant &other)
    {
        type = other.type;
        if      (!other.value)      value = NULL;
        else if (type == astType()) value = likely_retain_ast(other);
        else if (type == envType()) value = likely_retain_env(other);
        else                        value = likely_retain_mat(other);
        return *this;
    }

    operator bool() const { return value != NULL; }
    operator likely_const_ast() const { return (value && (type == astType())) ? ast : NULL; }
    operator likely_const_env() const { return (value && (type == envType())) ? env : NULL; }
    operator likely_const_mat() const { return (value && ((type != astType()) && (type != envType()))) ? mat : NULL; }

private:
    // We carefully avoid unnecessary calls to these functions in order to
    // sidestep the static initialization order fiasco with variables handling
    // pointer and struct types in frontend.cpp.
    static likely_type astType()
    {
        static likely_type ast = likely_pointer_type(likely_struct_type("ast", NULL, 0));
        return ast;
    }

    static likely_type envType()
    {
        static likely_type env = likely_pointer_type(likely_struct_type("env", NULL, 0));
        return env;
    }
};

typedef struct likely_expression *likely_expr;
typedef struct likely_expression const *likely_const_expr;

static likely_env newEnv(likely_const_env parent, likely_const_ast ast = NULL, likely_const_expr expr = NULL)
{
    likely_env env = (likely_env) malloc(sizeof(likely_environment));
    if (!env)
        return NULL;

    env->parent = likely_retain_env(parent);
    env->ast = likely_retain_ast(ast);
    if (parent) {
        env->settings = parent->settings;
        env->module = parent->module;
    } else {
        env->settings = (likely_settings*) malloc(sizeof(likely_settings));
        *env->settings = likely_default_settings(likely_file_void, false);
        env->module = NULL;
    }
    env->expr = expr;
    env->ref_count = 1;
    return env;
}

struct likely_expression : public LikelyValue
{
    mutable vector<likely_vtable> vtables;

    likely_expression(const LikelyValue &value = LikelyValue(), const Variant &data = Variant())
        : LikelyValue(value), data(data) {}

    virtual ~likely_expression(); // use release()
    likely_expression(const likely_expression &) = delete;
    likely_expression &operator=(const likely_expression &) = delete;

    static likely_const_expr retain(const likely_const_expr expr)
    {
        if (!expr)
            return NULL;
        assert(expr->ref_count > 0);
        if (expr->ref_count != std::numeric_limits<uint32_t>::max())
            expr->ref_count++;
        return expr;
    }

    static void release(const likely_const_expr expr)
    {
        if (!expr)
            return;
        assert(expr->ref_count > 0);
        if ((expr->ref_count == std::numeric_limits<uint32_t>::max()) || --expr->ref_count)
            return;
        delete expr;
    }

    virtual int uid() const { return 0; }
    virtual size_t maxParameters() const { return 0; }
    virtual size_t minParameters() const { return maxParameters(); }
    virtual const char *symbol() const { return ""; }

    virtual Variant getData() const
    {
        if (data || !value)
            return data;

        likely_mat m = NULL;
        if (ConstantInt *constantInt = dyn_cast<ConstantInt>(value)) {
            m = likely_new(type, 1, 1, 1, 1, NULL);
            likely_set_element(m, (type & likely_signed) ? double(constantInt->getSExtValue())
                                                         : double(constantInt->getZExtValue()), 0, 0, 0, 0);
        } else if (ConstantFP *constantFP = dyn_cast<ConstantFP>(value)) {
            const APFloat &apFloat = constantFP->getValueAPF();
            if ((&apFloat.getSemantics() == &APFloat::IEEEsingle) || (&apFloat.getSemantics() == &APFloat::IEEEdouble)) {
                // This should always be the case
                m = likely_new(type, 1, 1, 1, 1, NULL);
                likely_set_element(m, &apFloat.getSemantics() == &APFloat::IEEEsingle ? double(apFloat.convertToFloat())
                                                                                      : apFloat.convertToDouble(), 0, 0, 0, 0);
            }
        } else if (GEPOperator *gepOperator = dyn_cast<GEPOperator>(value)) {
            if (GlobalValue *globalValue = dyn_cast<GlobalValue>(gepOperator->getPointerOperand()))
                if (ConstantDataSequential *constantDataSequential = dyn_cast<ConstantDataSequential>(globalValue->getOperand(0)))
                    if (constantDataSequential->isCString())
                        m = likely_string(constantDataSequential->getAsCString().data());
        }

        data = Variant(m);
        return data;
    }

    virtual likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const;

    static size_t length(likely_const_ast ast)
    {
        return ast ? ((ast->type == likely_ast_list) ? ast->num_atoms : 1) : 0;
    }

    static void define(likely_const_env &env, const char *name, likely_const_expr expr)
    {
        const likely_ast atoms[2] = { likely_atom("=", 1), likely_atom(name, uint32_t(strlen(name))) };
        const likely_ast list = likely_list(atoms, 2);
        env = newEnv(env, list, expr);
        likely_release_ast(list);
    }

    static likely_const_expr get(Builder &builder, likely_const_ast ast);

protected:
    static likely_mat eval(const likely_const_ast ast, const likely_const_env parent, bool *ok)
    {
        likely_retain_ast(ast);
        const likely_ast list = likely_list(&const_cast<const likely_ast&>(ast), 1);
        const likely_const_env env = likely_eval(list, parent, NULL, NULL);
        likely_release_ast(list);
        const likely_mat result = likely_retain_mat(likely_result(env ? env->expr : NULL));
        likely_release_env(env);
        *ok = (result != NULL);
        return result;
    }

    static likely_type evalType(likely_const_ast ast, likely_const_env parent, bool *ok)
    {
        const likely_const_mat result = eval(ast, parent, ok);
        *ok &= (result->type == likely_u32);
        const likely_type type = (*ok) ? likely_type(likely_get_element(result, 0, 0, 0, 0)) : likely_type(likely_void);
        likely_release_mat(result);
        return type;
    }

    static int evalInt(likely_const_ast ast, likely_const_env parent, bool *ok)
    {
        const likely_const_mat result = eval(ast, parent, ok);
        const int val = (*ok) ? int(likely_get_element(result, 0, 0, 0, 0)) : 0;
        likely_release_mat(result);
        return val;
    }

    static string evalString(likely_const_ast ast, likely_const_env parent, bool *ok)
    {
        const likely_const_mat result = eval(ast, parent, ok);
        *ok &= (result->type == likely_text);
        const string str = (*ok) ? result->data : "";
        likely_release_mat(result);
        return str;
    }

    static likely_const_env lookup(likely_const_env env, const char *name)
    {
        while (env) {
            if (likely_is_definition(env->ast) && !strcmp(name, likely_symbol(env->ast)))
                return env;
            env = env->parent;
        }
        return NULL;
    }

    static likely_const_expr error(likely_const_ast ast, const char *message)
    {
        likely_throw(ast, message);
        return NULL;
    }

    static void undefine(likely_const_env &env, const char *name)
    {
        assert(likely_is_definition(env->ast));
        const char *const top = likely_symbol(env->ast);
        likely_ensure(!strcmp(name, top), "undefine variable mismatch, expected: %s but got %s", name, top);
        const likely_const_env old = env;
        env = env->parent;
        likely_release_env(old);
    }

    static void undefineAll(likely_const_env &env, likely_const_ast args, size_t start=0)
    {
        if (args->type == likely_ast_list) {
            for (size_t i=0; i<args->num_atoms-start; i++)
                undefine(env, args->atoms[args->num_atoms-i-1]->atom);
        } else {
            undefine(env, args->atom);
        }
    }

    static void DCE(Function &function)
    {
        legacy::FunctionPassManager FPM(function.getParent());
        FPM.add(createDeadInstEliminationPass());
        while (FPM.run(function)) {}
    }

    void setData(const Variant &data) const
    {
        this->data = data;
    }

private:
    mutable Variant data; // use getData() and setData()
    mutable uint32_t ref_count = 1;

    static likely_const_expr _get(Builder &builder, likely_const_ast ast);
};

struct UniqueExpression : public unique_ptr<const likely_expression, decltype(&likely_expression::release)>
{
    UniqueExpression(const likely_const_expr expr = NULL)
        : unique_ptr<const likely_expression, decltype(&likely_expression::release)>(expr, likely_expression::release) {}
};

struct likely_module
{
    unique_ptr<LikelyContext> context;
    Module *module;
    vector<pair<Variant,Constant*>> data;

    likely_module(const likely_settings &settings)
        : context(new LikelyContext(settings))
        , module(new Module("likely", context->context)) {}

    likely_module(const likely_const_mat bitcode)
        : context(new LikelyContext(likely_default_settings(likely_file_void, false)))
    {
        const ErrorOr<Module*> result = parseBitcodeFile(MemoryBufferRef(StringRef(bitcode->data, likely_bytes(bitcode)), "likely"), context->context);
        module = result ? result.get() : NULL;
    }

    virtual ~likely_module()
    {
        finalize();
    }

    likely_module(const likely_module &) = delete;
    likely_module &operator=(const likely_module &) = delete;

    void finalize()
    {
        if (module) {
            delete module;
            module = NULL;
        }
        context.reset();
    }
};

namespace {

class OfflineModule : public likely_module
{
    likely_mat *const output;
    const likely_file_type file_type;

public:
    OfflineModule(const likely_settings &settings, likely_mat *output, likely_file_type file_type)
        : likely_module(settings), output(output), file_type(file_type) {}

    ~OfflineModule()
    {
        // Inline constant mats as they won't be around after the program exits!
        int inlinedIndex = 0;
        for (const pair<Variant,Constant*> &datum : data) {
            bool used = false;
            for (User *user : datum.second->users())
                if (user->getNumUses() > 0) {
                    used = true;
                    break;
                }

            if (!used)
                continue;

            const likely_const_mat mat = datum.first;
            assert(mat);

            const uint64_t elements = uint64_t(mat->channels) * uint64_t(mat->columns) * uint64_t(mat->rows) * uint64_t(mat->frames);
            StructType *const structType = cast<StructType>(cast<PointerType>(context->toLLVM(mat->type, elements))->getElementType());
            const likely_type c_type = mat->type & likely_c_type;
            Constant *const values[7] = { ConstantInt::get(IntegerType::get(context->context, 8*sizeof(uint32_t)), numeric_limits<uint32_t>::max()),
                                          ConstantInt::get(IntegerType::get(context->context, 8*sizeof(uint32_t)), mat->type),
                                          ConstantInt::get(IntegerType::get(context->context, 8*sizeof(uint32_t)), mat->channels),
                                          ConstantInt::get(IntegerType::get(context->context, 8*sizeof(uint32_t)), mat->columns),
                                          ConstantInt::get(IntegerType::get(context->context, 8*sizeof(uint32_t)), mat->rows),
                                          ConstantInt::get(IntegerType::get(context->context, 8*sizeof(uint32_t)), mat->frames),
                                          (c_type == likely_u8)  ? ConstantDataArray::get(context->context, ArrayRef<uint8_t >((uint8_t*)  &mat->data, elements))
                                        : (c_type == likely_u16) ? ConstantDataArray::get(context->context, ArrayRef<uint16_t>((uint16_t*) &mat->data, elements))
                                        : (c_type == likely_u32) ? ConstantDataArray::get(context->context, ArrayRef<uint32_t>((uint32_t*) &mat->data, elements))
                                        : (c_type == likely_u64) ? ConstantDataArray::get(context->context, ArrayRef<uint64_t>((uint64_t*) &mat->data, elements))
                                        : (c_type == likely_u64) ? ConstantDataArray::get(context->context, ArrayRef<float   >((float*)    &mat->data, elements))
                                        :                          ConstantDataArray::get(context->context, ArrayRef<double  >((double*)   &mat->data, elements)) };
            Constant *const constantStruct = ConstantStruct::get(structType, values);

            stringstream name;
            name << "likely_inlined_mat_" << inlinedIndex++;
            GlobalVariable *const globalVariable = cast<GlobalVariable>(module->getOrInsertGlobal(name.str(), structType));
            globalVariable->setConstant(true);
            globalVariable->setInitializer(constantStruct);
            globalVariable->setLinkage(GlobalVariable::PrivateLinkage);
            globalVariable->setUnnamedAddr(true);

            vector<Instruction*> eraseLater;
            for (User *user : datum.second->users()) {
                for (User *supposedInstruction : user->users()) {
                    Instruction *const instruction = cast<Instruction>(supposedInstruction);
                    CastInst *const castInst = CastInst::CreatePointerBitCastOrAddrSpaceCast(globalVariable, user->getType(), "", instruction);
                    user->replaceAllUsesWith(castInst);
                    if (CallInst *const callInst = dyn_cast<CallInst>(instruction)) {
                        if (   (callInst->getCalledFunction()->getName() == "likely_retain_mat")
                            || (callInst->getCalledFunction()->getName() == "likely_release_mat"))
                            instruction->replaceAllUsesWith(castInst);
                            eraseLater.push_back(instruction);
                    }
                }
            }
            for (Instruction *instruction : eraseLater)
                instruction->eraseFromParent();
        }

        context->PM->run(*module);

        if (context->verbose)
            module->print(outs(), NULL);

        SmallVector<char, 0> data;
        raw_svector_ostream stream(data);
        if (file_type == likely_file_ir) {
            module->print(stream, NULL);
            stream << '\0';
        } else if (file_type == likely_file_bitcode) {
            WriteBitcodeToFile(module, stream);
        } else {
            legacy::PassManager pm;
            formatted_raw_ostream fos(stream);
            static TargetMachine *TM = LikelyContext::getTargetMachine(false);
            TM->addPassesToEmitFile(pm, fos, (file_type == likely_file_assembly) ? TargetMachine::CGFT_AssemblyFile : TargetMachine::CGFT_ObjectFile);
            pm.run(*module);
        }
        stream.flush();

        if (data.empty()) *output = NULL;
        else              *output = (file_type == likely_file_ir) ? likely_string(data.data())
                                                                  : likely_new(likely_u8 | likely_multi_channel, uint32_t(data.size()), 1, 1, 1, data.data());
    }
};

struct Builder : public IRBuilder<>
{
    likely_const_env env;
    likely_module *const module;

    Builder(likely_const_env env, likely_module *module)
        : IRBuilder<>(module ? module->context->context : getGlobalContext()), env(env), module(module) {}

    LikelyValue constant(uint64_t value, likely_type type = likely_u64)
    {
        const unsigned depth = unsigned(type & likely_depth);
        return LikelyValue(Constant::getIntegerValue(Type::getIntNTy(getContext(), depth), APInt(depth, value)), type);
    }

    LikelyValue constant(double value, likely_type type)
    {
        const size_t depth = type & likely_depth;
        if (type & likely_floating) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return LikelyValue(ConstantFP::get(Type::getDoubleTy(getContext()), value), type);
            else if (depth == 32) return LikelyValue(ConstantFP::get(Type::getFloatTy(getContext()), value), type);
            else                  { likely_ensure(false, "invalid floating point constant depth: %d", depth); return LikelyValue(); }
        } else {
            return constant(uint64_t(value), type);
        }
    }

    LikelyValue zero(likely_type type = likely_u64) { return constant(0.0, type); }
    LikelyValue one (likely_type type = likely_u64) { return constant(1.0, type); }

    LikelyValue intMax(likely_type type)
    {
        ConstantRange constantRange(type & likely_depth);
        const APInt apInt = (type & likely_signed) ? constantRange.getSignedMax() : constantRange.getUnsignedMax();
        Constant *const constant = ConstantInt::getIntegerValue(Type::getIntNTy(getContext(), type & likely_depth), apInt);
        return LikelyValue(constant, type);
    }

    LikelyValue intMin(likely_type type)
    {
        ConstantRange constantRange(type & likely_depth);
        const APInt apInt = (type & likely_signed) ? constantRange.getSignedMin() : constantRange.getUnsignedMin();
        Constant *const constant = ConstantInt::getIntegerValue(Type::getIntNTy(getContext(), type & likely_depth), apInt);
        return LikelyValue(constant, type);
    }

    LikelyValue nullMat() { return LikelyValue(ConstantPointerNull::get(::cast<PointerType>(module->context->toLLVM(likely_multi_dimension))), likely_multi_dimension); }
    LikelyValue nullData() { return LikelyValue(ConstantPointerNull::get(Type::getInt8PtrTy(getContext())), likely_pointer_type(likely_u8)); }

    Value *addInts(Value *lhs, Value *rhs, const Twine &name = "")
    {
        if (Constant *c = dyn_cast<Constant>(lhs))
            if (c->isZeroValue())
                return rhs;
        if (Constant *c = dyn_cast<Constant>(rhs))
            if (c->isZeroValue())
                return lhs;
        return CreateAdd(lhs, rhs, name, true, true);
    }

    Value *multiplyInts(Value *lhs, Value *rhs, const Twine &name = "")
    {
        if (Constant *const c = dyn_cast<Constant>(lhs)) {
            if (c->isZeroValue())
                return c;
            if (c->isOneValue())
                return rhs;
        }
        if (Constant *const c = dyn_cast<Constant>(rhs)) {
            if (c->isZeroValue())
                return c;
            if (c->isOneValue())
                return lhs;
        }
        return CreateMul(lhs, rhs, name, true, true);
    }

    LikelyValue channels(const LikelyValue &m)
    {
        if (!(m & likely_multi_channel))
            return one(likely_u32);
        return axis(m, 0, "channels");
    }

    LikelyValue columns(const LikelyValue &m)
    {
        if (!(m & likely_multi_column))
            return one(likely_u32);
        return axis(m, 1, "columns");
    }

    LikelyValue rows(const LikelyValue &m)
    {
        if (!(m & likely_multi_row))
            return one(likely_u32);
        return axis(m, 2, "rows");
    }

    LikelyValue frames(const LikelyValue &m)
    {
        if (!(m & likely_multi_frame))
            return one(likely_u32);
        return axis(m, 3, "frames");
    }

    LikelyValue data(const LikelyValue &m)
    {
        if (!likely_expression::isMat(m.value->getType()))
            return LikelyValue();
        const likely_type type = likely_pointer_type(m & likely_element);
        return LikelyValue(CreatePointerCast(CreateStructGEP(m, 6), module->context->toLLVM(type)), type);
    }

    enum Comparison
    {
        LT, // Less-than
        LE, // Less-than or equal-to
        GT, // Greater-than
        GE, // Greater-than or equal-to
        EQ, // Equal-to
        NE, // Not equal-to
    };

    LikelyValue compare(const LikelyValue &lhs, const LikelyValue &rhs, Comparison comparison)
    {
        Value *result = NULL;
        switch (comparison) {
          case LT:
            result = (lhs.type & likely_floating) ? CreateFCmpOLT(lhs, rhs)
                                                  : ((lhs.type & likely_signed) ? CreateICmpSLT(lhs, rhs)
                                                                                : CreateICmpULT(lhs, rhs));
            break;
          case LE:
            result = (lhs.type & likely_floating) ? CreateFCmpOLE(lhs, rhs)
                                                  : ((lhs.type & likely_signed) ? CreateICmpSLE(lhs, rhs)
                                                                                : CreateICmpULE(lhs, rhs));
            break;
          case GT:
            result = (lhs.type & likely_floating) ? CreateFCmpOGT(lhs, rhs)
                                                  : ((lhs.type & likely_signed) ? CreateICmpSGT(lhs, rhs)
                                                                                : CreateICmpUGT(lhs, rhs));
            break;
          case GE:
            result = (lhs.type & likely_floating) ? CreateFCmpOGE(lhs, rhs)
                                                  : ((lhs.type & likely_signed) ? CreateICmpSGE(lhs, rhs)
                                                                                : CreateICmpUGE(lhs, rhs));
            break;
          case EQ:
            result = (lhs.type & likely_floating) ? CreateFCmpOEQ(lhs, rhs)
                                                  : CreateICmpEQ(lhs, rhs);
            break;
          case NE:
            result = (lhs.type & likely_floating) ? CreateFCmpONE(lhs, rhs)
                                                  : CreateICmpNE(lhs, rhs);
            break;
        }

        return LikelyValue(result, likely_u1);
    }

    LikelyValue numericLimit(likely_type type, bool minimum)
    {
        const int depth = type & likely_depth;
        if (type & likely_floating) {
            if      (depth == 16) return LikelyValue(ConstantFP::get(getContext(), APFloat::getLargest(APFloat::IEEEhalf, minimum)), likely_f16);
            else if (depth == 32) return LikelyValue(ConstantFP::get(getContext(), APFloat::getLargest(APFloat::IEEEsingle, minimum)), likely_f32);
            else if (depth == 64) return LikelyValue(ConstantFP::get(getContext(), APFloat::getLargest(APFloat::IEEEdouble, minimum)), likely_f64);
            else    return LikelyValue();
        } else if (type & likely_signed) {
            return LikelyValue(ConstantInt::get(getContext(), minimum ? APInt::getSignedMinValue(depth) : APInt::getSignedMaxValue(depth)), depth | likely_signed);
        } else {
            return LikelyValue(ConstantInt::get(getContext(), minimum ? APInt::getMinValue(depth) : APInt::getMaxValue(depth)), depth);
        }
    }

    LikelyValue cast(const LikelyValue &x, likely_type type)
    {
        if (x.type == type)
            return x;

        // matrix -> matrix
        if ((x.type & likely_multi_dimension) && (type & likely_multi_dimension))
            return LikelyValue(CreatePointerCast(x, module->context->toLLVM(type)), type);

        // int -> pointer, int -> matrix
        if (!(x.type & (~likely_element | likely_floating))
            && ((type & likely_compound_pointer) || (type & likely_multi_dimension))) {
            Type *const dstType = module->context->toLLVM(type);
            return LikelyValue(CreateIntToPtr(x.value, dstType), type);
        }

        // scalar -> scalar
        assert(!(x.type & ~likely_element) && !(type & ~likely_element));
        if ((type & likely_depth) == 0) {
            type |= x.type & likely_depth;
            if (type & likely_floating)
                type = likely_type_from_types(type, likely_floating); // Promote to a valid floating point type
        }
        Type *const dstType = module->context->toLLVM(type & likely_element);
        const LikelyValue casted(CreateCast(CastInst::getCastOpcode(x, (x & likely_signed) != 0, dstType, (type & likely_signed) != 0), x, dstType), type);

        if (x.type & likely_saturated) {
            bool lossless;
            if (x.type & likely_floating) {
                if (type & likely_floating)
                    lossless = (x.type & likely_depth) <= (type & likely_depth);     // float -> float
                else
                    lossless = false;                                                // float -> int
            } else {
                if (type & likely_floating)
                    lossless = (x.type & likely_depth) <= (type & likely_depth) / 2; // int -> float
                else
                    lossless = ((x.type & likely_depth) < (type & likely_depth))     // int -> int
                            || (((x.type & likely_depth ) == (type & likely_depth))
                             && ((x.type & likely_signed) == (type & likely_signed)));
            }

            if (lossless)
                return casted;

            Value *result = casted;

            // The only time we don't need to check for underflow is unsigned -> signed integers of the same depth
            if (!((((x.type ^ type) & likely_c_type) == likely_signed) && (type & likely_signed))) {
                const LikelyValue numericMinimum = numericLimit(type, true);
                const LikelyValue exceedsMinimum = compare(x, cast(numericMinimum, x.type), LT);
                result = CreateSelect(exceedsMinimum, numericMinimum, result);
            }

            // The only time we don't need to check for overflow is signed -> unsigned integers of the same depth
            if (!((((x.type ^ type) & likely_c_type) == likely_signed) && !(type & likely_signed))) {
                const LikelyValue numericMaximum = numericLimit(type, false);
                const LikelyValue exceedsMaximum = compare(x, cast(numericMaximum, x.type), GT);
                result = CreateSelect(exceedsMaximum, numericMaximum, result);
            }

            return LikelyValue(result, type);
        } else {
            return casted;
        }
    }

    LikelyValue toMat(const LikelyValue &expr)
    {
        if (LikelyValue::isMat(expr.value->getType()))
            return expr;

        if (expr.value->getType()->isPointerTy() /* assume it's a string for now */) {
            Function *likelyString = module->module->getFunction("likely_string");
            if (!likelyString) {
                FunctionType *functionType = FunctionType::get(module->context->toLLVM(likely_i8 | likely_multi_channel), Type::getInt8PtrTy(getContext()), false);
                likelyString = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_string", module->module);
                likelyString->setCallingConv(CallingConv::C);
                likelyString->setDoesNotAlias(0);
                likelyString->setDoesNotAlias(1);
                likelyString->setDoesNotCapture(1);
                sys::DynamicLibrary::AddSymbol("likely_string", (void*) likely_string);
            }
            return LikelyValue(CreateCall(likelyString, expr), likely_i8 | likely_multi_channel);
        }

        Function *likelyScalar = module->module->getFunction("likely_scalar");
        if (!likelyScalar) {
            Type *params[] = { Type::getInt32Ty(getContext()), Type::getDoublePtrTy(getContext()), Type::getInt32Ty(getContext()) };
            FunctionType *functionType = FunctionType::get(module->context->toLLVM(likely_multi_dimension), params, false);
            likelyScalar = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_scalar", module->module);
            likelyScalar->setCallingConv(CallingConv::C);
            likelyScalar->setDoesNotAlias(0);
            likelyScalar->setDoesNotAlias(2);
            likelyScalar->setDoesNotCapture(2);
            sys::DynamicLibrary::AddSymbol("likely_scalar", (void*) likely_scalar);
        }

        AllocaInst *allocaInst = CreateAlloca(Type::getDoubleTy(getContext()), one());
        CreateStore(cast(expr, likely_f64), allocaInst);
        return LikelyValue(CreateCall3(likelyScalar, constant(uint64_t(expr.type), likely_u32), allocaInst, one(likely_u32)), likely_multi_dimension);
    }

    LikelyValue retainMat(Value *m)
    {
        Function *likelyRetain = module->module->getFunction("likely_retain_mat");
        if (!likelyRetain) {
            FunctionType *functionType = FunctionType::get(module->context->toLLVM(likely_multi_dimension), module->context->toLLVM(likely_multi_dimension), false);
            likelyRetain = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_retain_mat", module->module);
            likelyRetain->setCallingConv(CallingConv::C);
            likelyRetain->setDoesNotAlias(1);
            likelyRetain->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("likely_retain_mat", (void*) likely_retain_mat);
        }
        return LikelyValue(CreateCall(likelyRetain, CreatePointerCast(m, module->context->toLLVM(likely_multi_dimension))), likely_multi_dimension);
    }

    LikelyValue releaseMat(Value *m)
    {
        Function *likelyRelease = module->module->getFunction("likely_release_mat");
        if (!likelyRelease) {
            FunctionType *functionType = FunctionType::get(Type::getVoidTy(getContext()), module->context->toLLVM(likely_multi_dimension), false);
            likelyRelease = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_release_mat", module->module);
            likelyRelease->setCallingConv(CallingConv::C);
            likelyRelease->setDoesNotAlias(1);
            likelyRelease->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("likely_release_mat", (void*) likely_release_mat);
        }
        return LikelyValue(CreateCall(likelyRelease, CreatePointerCast(m, module->context->toLLVM(likely_multi_dimension))), likely_void);
    }

    void assume(Value *const condition)
    {
        Function *const assume = Intrinsic::getDeclaration(module->module, Intrinsic::assume);
        CallInst *const assumption = CreateCall(assume, condition);
        module->context->ACT->getAssumptionCache(*GetInsertBlock()->getParent()).registerAssumption(assumption);
    }

private:
    LikelyValue axis(Value *m, const unsigned idx, const char *const name)
    {
        // Trace back through casts
        while (CastInst *const cast = dyn_cast<CastInst>(m))
            m = cast->getOperand(0);

        // If the origin is a call to "likely_new" then we can determine the
        // axis value from the corresponding function call argument.
        if (CallInst *const call = dyn_cast<CallInst>(m))
            if (call->getCalledFunction()->getName() == "likely_new")
                return LikelyValue(call->getOperand(idx+1), likely_u32);

        // See if we have already indexed it
        Value *priorGEP = NULL;
        for (BasicBlock &BB : *GetInsertBlock()->getParent())
            for (Instruction &I : BB)
                if (GetElementPtrInst *const GEP = dyn_cast<GetElementPtrInst>(&I))
                    if (GEP->getOperand(0) == m)
                        if (ConstantInt *const idx1 = dyn_cast<ConstantInt>(GEP->getOperand(1)))
                            if (idx1->isZero())
                                if (ConstantInt *const idx2 = dyn_cast<ConstantInt>(GEP->getOperand(2)))
                                    if (idx2->equalsInt(idx+2))
                                        priorGEP = GEP;

        // See if we have already loaded it
        if (priorGEP)
            for (User *const user : priorGEP->users())
                if (LoadInst *const load = dyn_cast<LoadInst>(user))
                    return LikelyValue(load, likely_u32);

        if (!priorGEP)
            priorGEP = CreateStructGEP(m, idx+2);

        LoadInst *const load = CreateLoad(priorGEP, name);
        load->setMetadata(LLVMContext::MD_range, axisRange());
        return LikelyValue(load, likely_u32);
    }

    MDNode *axisRange()
    {
        Type *const type = Type::getInt32Ty(getContext());
        Metadata *const values[2] = { ConstantAsMetadata::get(ConstantInt::get(type, 1)),
                                      ConstantAsMetadata::get(ConstantInt::get(type, std::numeric_limits<uint32_t>::max())) };
        return MDNode::get(getContext(), values);
    }
};

struct ConstantString : public likely_expression
{
    ConstantString(Builder &builder, const likely_const_mat str)
    {
        setData(str);
        value = builder.CreateGlobalStringPtr(str->data);
        type = likely_pointer_type(likely_i8);
    }

private:
    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        assert(ast->type != likely_ast_list);
        return new ConstantString(builder, likely_retain_mat(getData()));
    }
};

struct ConstantData : public likely_expression
{
    ConstantData(const Variant &data)
        : likely_expression(LikelyValue(), data) {}

    ConstantData(Builder &builder, const Variant &data)
    {
        if (!data)
            return;

        setData(data);
        type = data.type;

        // Special case, return the scalar
        if (const likely_const_mat m = data)
            if (!(m->type & likely_multi_dimension)) {
                value = builder.constant(likely_get_element(m, 0, 0, 0, 0), m->type);
                return;
            }

        // Make sure the lifetime of the data is at least as long as the lifetime of the code.
        Constant *const address = ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(void*)), uintptr_t(data.value));
        builder.module->data.push_back(pair<Variant,Constant*>(data, address));
        value = ConstantExpr::getIntToPtr(address, builder.module->context->toLLVM(data.type));
    }

private:
    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (ast->type == likely_ast_list)
            return likely_expression::evaluate(builder, ast); // Knows how to index into the matrix
        return new ConstantData(builder, getData());
    }
};

#define TRY_EXPR(BUILDER, AST, EXPR)            \
const UniqueExpression EXPR(get(BUILDER, AST)); \
if (!EXPR.get()) return NULL;                   \

class LikelyOperator : public likely_expression
{
    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (ast->type != likely_ast_list)
            return retain(this);

        const size_t args = length(ast)-1;
        const size_t min = minParameters();
        const size_t max = maxParameters();
        if ((args < min) || (args > max)) {
            stringstream stream;
            stream << "operator with: " << min;
            if (max != min)
                stream << "-" << max;
            stream << " parameters passed: " << args << " argument" << (args == 1 ? "" : "s");
            return error(ast, stream.str().c_str());
        }

        return evaluateOperator(builder, ast);
    }

    virtual likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const = 0;
};

struct LikelyFunction : public LikelyOperator
{
    enum CallingConvention
    {
        RegularCC,
        ArrayCC,
        VirtualCC
    };

    const likely_const_env env;

    LikelyFunction(const likely_const_env env, const size_t numParameters)
        : env(likely_retain_env(env))
        , numParameters(numParameters) {}

    ~LikelyFunction()
    {
        likely_release_env(env);
    }

    static bool isSymbol(const char *symbol)
    {
        return !strcmp(symbol, "->") || !strcmp(symbol, "extern");
    }

    Variant evaluateConstantFunction(const vector<likely_const_mat> &args = vector<likely_const_mat>()) const;

    likely_const_expr generate(Builder &builder, vector<likely_type> parameters, string name, bool promoteScalarToMatrix, CallingConvention cc, const vector<likely_type> &virtualTypes = vector<likely_type>()) const
    {
        assert(cc == VirtualCC ? !virtualTypes.empty() : virtualTypes.empty());
        while (parameters.size() < maxParameters())
            parameters.push_back(likely_multi_dimension);

        vector<Type*> llvmTypes;
        if (cc == RegularCC) {
            for (const likely_type &parameter : parameters)
                llvmTypes.push_back(builder.module->context->toLLVM(parameter));
        } else if (cc == ArrayCC) {
            // Array calling convention - All arguments come stored in an array of matricies.
            llvmTypes.push_back(PointerType::getUnqual(builder.module->context->toLLVM(likely_multi_dimension)));
        } else if (cc == VirtualCC) {
            // Virtual calling convention - Dynamically typed arguments come stored in an array of matricies.
            //                              Statically typed arguments come stored in a struct pointer.
            llvmTypes.push_back(PointerType::getUnqual(builder.module->context->toLLVM(likely_multi_dimension)));
            llvmTypes.push_back(PointerType::getUnqual(getStaticDataType(builder.module->context.get(), virtualTypes)));
        } else {
            assert(!"Invalid calling convention!");
        }

        BasicBlock *originalInsertBlock = builder.GetInsertBlock();
        Function *tmpFunction = cast<Function>(builder.module->module->getOrInsertFunction(name+"_tmp", FunctionType::get(Type::getVoidTy(builder.getContext()), llvmTypes, false)));
        BasicBlock *entry = BasicBlock::Create(builder.getContext(), "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        vector<likely_const_expr> arguments;
        if (cc == RegularCC) {
            Function::arg_iterator it = tmpFunction->arg_begin();
            size_t i = 0;
            while (it != tmpFunction->arg_end())
                arguments.push_back(new likely_expression(LikelyValue(it++, parameters[i++])));
        } else if (cc == ArrayCC) {
            Value *const argumentArray = tmpFunction->arg_begin();
            for (size_t i=0; i<parameters.size(); i++) {
                Value *load = builder.CreateLoad(builder.CreateGEP(argumentArray, builder.constant(i)));
                load = castOrPromote(builder, load, parameters[i]);
                arguments.push_back(new likely_expression(LikelyValue(load, parameters[i])));
            }
        } else if (cc == VirtualCC) {
            Function::arg_iterator it = tmpFunction->arg_begin();
            Value *const dynamicArguments = it++;
            Value *const staticArguments = it++;
            int dynamicIndex = 0, staticIndex = 0;
            for (size_t i=0; i<parameters.size(); i++) {
                if (virtualTypes[i] == likely_multi_dimension) {
                    Value *load = builder.CreateLoad(builder.CreateGEP(dynamicArguments, builder.constant(dynamicIndex++)));
                    load = castOrPromote(builder, load, parameters[i]);
                    arguments.push_back(new likely_expression(LikelyValue(load, parameters[i])));
                } else {
                    arguments.push_back(new likely_expression(LikelyValue(builder.CreateLoad(builder.CreateStructGEP(staticArguments, staticIndex++)), parameters[i])));
                }
            }
        } else {
            assert(!"Invalid calling convention!");
        }

        for (size_t i=0; i<arguments.size(); i++) {
            stringstream name; name << "arg_" << i;
            arguments[i]->value->setName(name.str());
        }

        UniqueExpression result(evaluateFunction(builder, arguments));
        if (!result)
            return NULL;

        if (result->type) {
            assert(result->value);

            // If we are expecting a constant or a matrix and don't get one then make a matrix
            if (promoteScalarToMatrix && !result->getData() && !dyn_cast<PointerType>(result->value->getType()))
                result.reset(new likely_expression(builder.toMat(*result)));

            // If we are returning a constant matrix, make sure to retain a copy
            if (isa<ConstantExpr>(result->value) && isMat(result->value->getType()))
                result.reset(new likely_expression(LikelyValue(builder.CreatePointerCast(builder.retainMat(result->value), builder.module->context->toLLVM(result->type)), result->type), result->getData()));

            builder.CreateRet(*result);
        } else {
            assert(!result->value);
            assert(!promoteScalarToMatrix);
            builder.CreateRetVoid();
        }

        Function *const function = cast<Function>(builder.module->module->getOrInsertFunction(name, FunctionType::get(builder.module->context->toLLVM(result->type), llvmTypes, false)));

        ValueToValueMapTy VMap;
        Function::arg_iterator tmpArgs = tmpFunction->arg_begin();
        Function::arg_iterator args = function->arg_begin();
        while (args != function->arg_end())
            VMap[tmpArgs++] = args++;

        SmallVector<ReturnInst*, 1> returns;
        CloneFunctionInto(function, tmpFunction, VMap, false, returns);
        tmpFunction->eraseFromParent();

        if (originalInsertBlock)
            builder.SetInsertPoint(originalInsertBlock);

        return new likely_expression(LikelyValue(function, result->type), result->getData());
    }

protected:
    const size_t numParameters;

    size_t maxParameters() const { return numParameters; }

    static StructType *getStaticDataType(LikelyContext *context, const vector<likely_type> &types)
    {
        vector<Type*> elements;
        for (const likely_type type : types)
            if (type != likely_multi_dimension)
                elements.push_back(context->toLLVM(type));
        return StructType::get(context->context, elements);
    }

    static Value *castOrPromote(Builder &builder, Value *load, likely_type type)
    {
        if (type & likely_multi_dimension) {
            return builder.CreatePointerCast(load, builder.module->context->toLLVM(type));
        } else {
            const likely_type tmpType = type | likely_multi_dimension;
            const LikelyValue tmpValue(builder.CreatePointerCast(load, builder.module->context->toLLVM(tmpType)), tmpType);
            return builder.CreateLoad(builder.CreateGEP(builder.data(tmpValue), builder.zero()));
        }
    }

private:
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_expr result = NULL;

        vector<likely_const_expr> args;
        const size_t arguments = length(ast)-1;
        for (size_t i=0; i<arguments; i++) {
            const likely_const_expr arg = get(builder, ast->atoms[i+1]);
            if (!arg)
                goto cleanup;
            args.push_back(arg);
        }

        result = evaluateFunction(builder, args);

    cleanup:
        for (likely_const_expr arg : args)
            release(arg);
        return result;
    }

    virtual likely_const_expr evaluateFunction(Builder &builder, vector<likely_const_expr> &args /* takes ownership */) const = 0;
};

struct Symbol : public LikelyFunction
{
    const string name;
    const vector<likely_type> parameters;

    Symbol(const likely_const_env env, const string &name, const likely_type returnType, const vector<likely_type> parameters = vector<likely_type>())
        : LikelyFunction(env, parameters.size())
        , name(name)
        , parameters(parameters)
    {
        type = returnType;
    }

private:
    likely_const_expr evaluateFunction(Builder &builder, vector<likely_const_expr> &args) const
    {
        Function *symbol = builder.module->module->getFunction(name);
        if (!symbol) {
            vector<Type*> llvmParameters;
            for (likely_type parameter : parameters)
                llvmParameters.push_back(builder.module->context->toLLVM(parameter));
            // If the return type is a matrix, we generalize it to allow overloading.
            Type *llvmReturn = builder.module->context->toLLVM(type & likely_multi_dimension ? likely_type(likely_multi_dimension) : type);
            FunctionType *functionType = FunctionType::get(llvmReturn, llvmParameters, false);
            symbol = Function::Create(functionType, GlobalValue::ExternalLinkage, name, builder.module->module);
            symbol->setCallingConv(CallingConv::C);
            symbol->setDoesNotThrow();
            if (name == "likely_new")
                symbol->setOnlyReadsMemory();
            if (isa<PointerType>(llvmReturn)) {
                symbol->setDoesNotAlias(0);
            } else if (isa<IntegerType>(llvmReturn)) {
                symbol->addAttribute(0, type & likely_signed ? Attribute::SExt : Attribute::ZExt);
            }
            for (size_t i=0; i<llvmParameters.size(); i++) {
                if (isa<PointerType>(llvmParameters[i])) {
                    symbol->setDoesNotAlias(unsigned(i)+1);
                    symbol->setDoesNotCapture(unsigned(i)+1);
                } else if (isa<IntegerType>(llvmParameters[i])) {
                    symbol->addAttribute(unsigned(i)+1, parameters[i] & likely_signed ? Attribute::SExt : Attribute::ZExt);
                }
            }
        }

        vector<Value*> castedArgs;
        for (size_t i=0; i<args.size(); i++) {
            const likely_const_expr arg = args[i];
            const likely_type parameter = parameters[i];
            if (arg->type & likely_multi_dimension) {
                if (parameter & likely_multi_dimension)
                    castedArgs.push_back(builder.CreatePointerCast(*arg, builder.module->context->toLLVM(parameter)));
                else if (parameter & likely_compound_pointer)
                    castedArgs.push_back(builder.CreatePointerCast(builder.data(*arg), builder.module->context->toLLVM(parameter)));
                else
                    likely_ensure(false, "can't cast matrix to scalar");
            } else {
                castedArgs.push_back(builder.cast(*arg, parameter));
            }
        }

        Value *value = builder.CreateCall(symbol, castedArgs);
        if (type & likely_multi_dimension)
            value = builder.CreatePointerCast(value, builder.module->context->toLLVM(type));
        return new likely_expression(LikelyValue(value, type));
    }
};

struct JITFunction : public Symbol
{
    void *function = NULL;
    ExecutionEngine *EE = NULL;
    likely_module *module;

    JITFunction(const string &name, const LikelyFunction *function, const vector<likely_type> &parameters, bool evaluate, LikelyFunction::CallingConvention cc, const vector<likely_type> &virtualTypes = vector<likely_type>())
        : Symbol(NULL, name, likely_void, parameters)
        , module(new likely_module(evaluate ? likely_default_settings(likely_file_void, function->env->settings->verbose) : *function->env->settings))
    {
        Builder builder(function->env, module);
        init(builder, name, function, parameters, evaluate, cc, virtualTypes);
    }

    JITFunction(const string &name, likely_const_ast ast, likely_const_env env, const vector<likely_type> &parameters, LikelyFunction::CallingConvention cc)
        : Symbol(NULL, name, likely_void, parameters)
        , module(new likely_module(*env->settings))
    {
        Builder builder(env, module);
        const UniqueExpression function(likely_expression::get(builder, ast));
        init(builder, name, reinterpret_cast<const LikelyFunction*>(function.get()), parameters, false, cc);
    }

    JITFunction(const likely_const_mat bitcode, const char *symbol)
        : Symbol(NULL, symbol, likely_void)
        , module(new likely_module(bitcode))
    {
        if (module->module) {
            EE = createExecutionEngine(unique_ptr<Module>(module->module), EngineKind::JIT);
            module->context->PM->run(*module->module);
            compileAndCleanup();
        } else {
            function = NULL;
        }
    }

    ~JITFunction()
    {
        if (EE && module->module) // interpreter
            EE->removeModule(module->module);
        delete EE;
        delete module;
    }

    static void *getFunction(likely_const_expr expr)
    {
        if (!expr || (expr->uid() != UID()))
            return NULL;
        return reinterpret_cast<const JITFunction*>(expr)->function;
    }

private:
    static int UID() { return __LINE__; }
    int uid() const { return UID(); }

    static ExecutionEngine *createExecutionEngine(unique_ptr<Module> module, EngineKind::Kind kind)
    {
        TargetMachine *const targetMachine = LikelyContext::getTargetMachine(true);
        module->setDataLayout(*targetMachine->getDataLayout());
        module->setTargetTriple(sys::getProcessTriple());

        string error;
        EngineBuilder engineBuilder(unique_ptr<Module>(module.release()));
        engineBuilder.setErrorStr(&error)
                     .setEngineKind(kind);

        ExecutionEngine *const EE = engineBuilder.create(targetMachine);
        likely_ensure(EE != NULL, "failed to create execution engine with error: %s", error.c_str());
        return EE;
    }

    void init(Builder &builder, const string &name, const LikelyFunction *function, const vector<likely_type> &parameters, bool evaluate, LikelyFunction::CallingConvention cc, const vector<likely_type> &virtualTypes = vector<likely_type>())
    {
        const UniqueExpression expr(function->generate(builder, parameters, name, evaluate, cc, virtualTypes));
        if (!expr) // error
            return;

        value = expr->value;
        type = expr->type;
        setData(expr->getData());
        if (evaluate && getData()) { // constant
            if (module->context->verbose)
                module->module->print(outs(), NULL);
            return;
        }

        // No libffi support for Windows
#ifdef _WIN32
        evaluate = false;
#endif // _WIN32

        // Don't run the interpreter on a module with loops, better to compile and execute it instead.
        if (evaluate) {
            legacy::PassManager PM;
            HasLoop *hasLoop = new HasLoop();
            PM.add(hasLoop);
            PM.run(*module->module);
            evaluate = !hasLoop->hasLoop;
        }

        EE = createExecutionEngine(unique_ptr<Module>(module->module),
                                   evaluate ? EngineKind::Interpreter : EngineKind::JIT);

        if (!evaluate) {
            // optimize
            EE->setObjectCache(&TheJITFunctionCache);
            if (!TheJITFunctionCache.alert(module->module))
                module->context->PM->run(*module->module);
        }

        if (module->context->verbose)
            module->module->print(outs(), NULL);

        if (!evaluate)
            compileAndCleanup();
    }

    void compileAndCleanup()
    {
        EE->finalizeObject();
        function = (void*) EE->getFunctionAddress(name.c_str());
        likely_ensure(function != NULL, "no function named: %s", name.c_str());
        EE->removeModule(module->module);
        module->finalize();
        value = NULL;
    }

    struct HasLoop : public LoopInfoWrapperPass
    {
        bool hasLoop = false;

    private:
        bool runOnFunction(Function &F)
        {
            if (hasLoop)
                return false;

            const bool result = LoopInfoWrapperPass::runOnFunction(F);
            hasLoop = !getLoopInfo().empty();
            return result;
        }
    };
};

Variant LikelyFunction::evaluateConstantFunction(const vector<likely_const_mat> &args) const
{
    vector<likely_type> params;
    for (const likely_const_mat arg : args)
        params.push_back(arg->type);

    JITFunction jit("likely_ctfe", this, params, true, args.empty() ? LikelyFunction::RegularCC : LikelyFunction::ArrayCC);
    void *value;
    if (jit.function) { // compiler
        value = args.empty() ? reinterpret_cast<void *(*)()>(jit.function)()
                             : reinterpret_cast<void *(*)(likely_const_mat const*)>(jit.function)(args.data());
    } else if (jit.EE) { // interpreter
        vector<GenericValue> gv;
        if (!args.empty())
            gv.push_back(GenericValue((void*) args.data()));
        value = jit.EE->runFunction(cast<Function>(jit.value), gv).PointerVal;
    } else { // constant or error
        value = likely_retain_mat(jit.getData());
    }
    return Variant(value, jit.type);
}

} // namespace (anonymous)

struct likely_virtual_table
{
    const likely_const_env env;
    const likely_const_ast body, parameters;
    const vector<likely_type> types;
    const size_t n;
    vector<unique_ptr<JITFunction>> functions;

    likely_virtual_table(likely_const_env env, likely_const_ast body, likely_const_ast parameters, const vector<likely_type> &types)
        : env(env) /* don't retain a copy because `env` owns us` */, body(likely_retain_ast(body)), parameters(likely_retain_ast(parameters)), types(types), n(likely_expression::length(parameters)) {}

    ~likely_virtual_table()
    {
        likely_release_ast(parameters);
        likely_release_ast(body);
    }
};

namespace {

struct RootEnvironment
{
    // Provide public access to an environment that includes the standard library.
    static likely_const_env get()
    {
        static bool init = false;
        if (!init) {
            builtins() = likely_lex_parse_and_eval(likely_standard_library, likely_file_tex, builtins());
            init = true;
        }

        return builtins();
    }

protected:
    // Provide protected access for registering builtins.
    static likely_const_env &builtins()
    {
        static likely_const_env root = NULL;
        if (!root)
            root = newEnv(NULL);
        return root;
    }
};

template <class E>
struct RegisterExpression : public RootEnvironment
{
    RegisterExpression()
    {
        likely_expr e = new E();
        likely_expression::define(builtins(), e->symbol(), e);
    }
};
#define LIKELY_REGISTER(EXP) static RegisterExpression<EXP##Expression> Register##EXP##Expression;

class Operand : public likely_expression
{
    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (ast->type == likely_ast_list)
            return error(ast, "operand does not callable");
        return evaluateOperand(builder);
    }
    virtual likely_const_expr evaluateOperand(Builder &builder) const = 0;
};

class UnaryOperator : public LikelyOperator
{
    size_t maxParameters() const { return 1; }
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        return evaluateUnary(builder, ast->atoms[1]);
    }
    virtual likely_const_expr evaluateUnary(Builder &builder, likely_const_ast arg) const = 0;
};

class SimpleUnaryOperator : public UnaryOperator
{
    likely_const_expr evaluateUnary(Builder &builder, likely_const_ast arg) const
    {
        TRY_EXPR(builder, arg, expr)
        return evaluateSimpleUnary(builder, expr);
    }
    virtual likely_const_expr evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const = 0;
};

struct MatrixType : public likely_expression
{
    likely_type t;
    MatrixType(Builder &builder, likely_type t)
        : t(t)
    {
        value = builder.constant(uint64_t(t), likely_u32);
        type = likely_u32;
    }

    static bool is(likely_const_expr expr)
    {
        return expr->uid() == UID();
    }

private:
    static int UID() { return __LINE__; }
    int uid() const { return UID(); }

    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        if ((ast->type == likely_ast_list) && (ast->num_atoms > 1)) {
            TRY_EXPR(builder, ast->atoms[1], expr)
            if (is(expr.get())) {
                const MatrixType *matrixType = static_cast<const MatrixType*>(expr.get());
                return new MatrixType(builder, likely_type_from_types(matrixType->t, t));
            } else {
                return new likely_expression(builder.cast(*expr, t));
            }
        } else {
            return new MatrixType(builder, t);
        }
    }
};

class pointerExpression : public UnaryOperator
{
    const char *symbol() const { return "pointer"; }
    likely_const_expr evaluateUnary(Builder &builder, likely_const_ast arg) const
    {
        bool ok;
        const likely_type elementType = evalType(arg, builder.env, &ok);
        if (!ok)
            return NULL;
        return new MatrixType(builder, likely_pointer_type(elementType));
    }
};
LIKELY_REGISTER(pointer)

class structExpression : public LikelyOperator
{
    size_t minParameters() const { return 1; }
    size_t maxParameters() const { return std::numeric_limits<uint32_t>::max(); }
    const char *symbol() const { return "struct"; }
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        bool ok;
        const string name = evalString(ast->atoms[1], builder.env, &ok);
        if (!ok)
            return NULL;
        vector<likely_type> types;
        for (uint32_t i=2; i<ast->num_atoms; i++) {
            types.push_back(evalType(ast->atoms[i], builder.env, &ok));
            if (!ok)
                return NULL;
        }
        return new MatrixType(builder, likely_struct_type(name.c_str(), types.empty() ? NULL : types.data(), int32_t(types.size())));
    }
};
LIKELY_REGISTER(struct)

#define LIKELY_REGISTER_AXIS(AXIS)                                                   \
class AXIS##Expression : public LikelyOperator                                       \
{                                                                                    \
    const char *symbol() const { return #AXIS; }                                     \
    size_t maxParameters() const { return 1; }                                       \
                                                                                     \
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const \
    {                                                                                \
        TRY_EXPR(builder, ast->atoms[1], expr)                                       \
        return new likely_expression(builder.AXIS(*expr.get()));                     \
    }                                                                                \
};                                                                                   \
LIKELY_REGISTER(AXIS)                                                                \

LIKELY_REGISTER_AXIS(channels)
LIKELY_REGISTER_AXIS(columns)
LIKELY_REGISTER_AXIS(rows)
LIKELY_REGISTER_AXIS(frames)
LIKELY_REGISTER_AXIS(data)

class notExpression : public SimpleUnaryOperator
{
    const char *symbol() const { return "~"; }
    likely_const_expr evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new likely_expression(LikelyValue(builder.CreateXor(builder.intMax(*arg), arg->value), *arg));
    }
};
LIKELY_REGISTER(not)

class typeExpression : public SimpleUnaryOperator
{
    const char *symbol() const { return "type"; }
    likely_const_expr evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new MatrixType(builder, *arg);
    }
};
LIKELY_REGISTER(type)

class makeTypeExpression : public SimpleUnaryOperator
{
    const char *symbol() const { return "make-type"; }
    likely_const_expr evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        if (ConstantInt *const constantInt = dyn_cast<ConstantInt>(arg->value))
            return new MatrixType(builder, likely_type(constantInt->getZExtValue()));
        return NULL;
    }
};
LIKELY_REGISTER(makeType)

class thisExpression : public Operand
{
    const char *symbol() const { return "this"; }
    likely_const_expr evaluateOperand(Builder &builder) const
    {
        return new ConstantData(builder, likely_retain_env(builder.env));
    }
};
LIKELY_REGISTER(this)

class UnaryMathOperator : public SimpleUnaryOperator
{
    likely_const_expr evaluateSimpleUnary(Builder &builder, const UniqueExpression &x) const
    {
        likely_expression xc(builder.cast(*x.get(), likely_type_from_types(*x, likely_floating)));
        return new likely_expression(LikelyValue(builder.CreateCall(Intrinsic::getDeclaration(builder.module->module, id(), xc.value->getType()), xc), xc));
    }
    virtual Intrinsic::ID id() const = 0;
};

#define LIKELY_REGISTER_UNARY_MATH(OP)                 \
class OP##Expression : public UnaryMathOperator        \
{                                                      \
    const char *symbol() const { return #OP; }         \
    Intrinsic::ID id() const { return Intrinsic::OP; } \
};                                                     \
LIKELY_REGISTER(OP)                                    \

LIKELY_REGISTER_UNARY_MATH(sqrt)
LIKELY_REGISTER_UNARY_MATH(sin)
LIKELY_REGISTER_UNARY_MATH(cos)
LIKELY_REGISTER_UNARY_MATH(exp)
LIKELY_REGISTER_UNARY_MATH(exp2)
LIKELY_REGISTER_UNARY_MATH(log)
LIKELY_REGISTER_UNARY_MATH(log10)
LIKELY_REGISTER_UNARY_MATH(log2)
LIKELY_REGISTER_UNARY_MATH(floor)
LIKELY_REGISTER_UNARY_MATH(ceil)
LIKELY_REGISTER_UNARY_MATH(trunc)
LIKELY_REGISTER_UNARY_MATH(round)

class SimpleBinaryOperator : public LikelyOperator
{
    size_t maxParameters() const { return 2; }
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[1], expr1)
        TRY_EXPR(builder, ast->atoms[2], expr2)
        return evaluateSimpleBinary(builder, expr1, expr2);
    }
    virtual likely_const_expr evaluateSimpleBinary(Builder &builder, const UniqueExpression &arg1, const UniqueExpression &arg2) const = 0;
};

class numericLimitExpression : public SimpleBinaryOperator
{
    const char *symbol() const { return "numeric-limit"; }
    likely_const_expr evaluateSimpleBinary(Builder &builder, const UniqueExpression &arg1, const UniqueExpression &arg2) const
    {
        if (!isa<ConstantInt>(arg1->value) || !isa<ConstantInt>(arg2->value))
            return NULL;
        const likely_type type = likely_type(cast<ConstantInt>(arg1->value)->getZExtValue());
        const bool minimum = cast<ConstantInt>(arg2->value)->getZExtValue() == 0;
        return new likely_expression(builder.numericLimit(type, minimum));
    }
};
LIKELY_REGISTER(numericLimit)

class ArithmeticOperator : public SimpleBinaryOperator
{
    likely_const_expr evaluateSimpleBinary(Builder &builder, const UniqueExpression &lhs, const UniqueExpression &rhs) const
    {
        const likely_type type = likely_type_from_types(*lhs, *rhs);
        return evaluateArithmetic(builder, builder.cast(*lhs.get(), type), builder.cast(*rhs.get(), type));
    }
    virtual likely_const_expr evaluateArithmetic(Builder &builder, const LikelyValue &lhs, const LikelyValue &rhs) const = 0;
};

class SimpleArithmeticOperator : public ArithmeticOperator
{
    likely_const_expr evaluateArithmetic(Builder &builder, const LikelyValue &lhs, const LikelyValue &rhs) const
    {
        return new likely_expression(LikelyValue(evaluateSimpleArithmetic(builder, lhs, rhs), lhs));
    }
    virtual Value *evaluateSimpleArithmetic(Builder &builder, const LikelyValue &lhs, const LikelyValue &rhs) const = 0;
};

class addExpression : public SimpleArithmeticOperator
{
    const char *symbol() const { return "+"; }
    Value *evaluateSimpleArithmetic(Builder &builder, const LikelyValue &lhs, const LikelyValue &rhs) const
    {
        if (lhs.type & likely_floating) {
            return builder.CreateFAdd(lhs, rhs);
        } else {
            if (lhs.type & likely_saturated) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module->module, (lhs.type & likely_signed) ? Intrinsic::sadd_with_overflow : Intrinsic::uadd_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = (lhs.type & likely_signed) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, builder.zero(lhs)), builder.intMax(lhs), builder.intMin(lhs)) : builder.intMax(lhs).value;
                return builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0));
            } else {
                return builder.addInts(lhs, rhs);
            }
        }
    }
};
LIKELY_REGISTER(add)

class subtractExpression : public LikelyOperator
{
    const char *symbol() const { return "-"; }
    size_t maxParameters() const { return 2; }
    size_t minParameters() const { return 1; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        UniqueExpression expr1, expr2;
        expr1.reset(get(builder, ast->atoms[1]));
        if (!expr1.get())
            return NULL;

        if (ast->num_atoms == 2) {
            // Unary negation
            expr2.reset(new likely_expression(builder.zero(*expr1)));
            expr2.swap(expr1);
        } else {
            // Binary subtraction
            expr2.reset(get(builder, ast->atoms[2]));
            if (!expr2.get())
                return NULL;
        }

        const likely_type type = likely_type_from_types(*expr1, *expr2);
        const likely_expression lhs(builder.cast(*expr1.get(), type));
        const likely_expression rhs(builder.cast(*expr2.get(), type));

        if (type & likely_floating) {
            return new likely_expression(LikelyValue(builder.CreateFSub(lhs, rhs), type));
        } else {
            if (type & likely_saturated) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module->module, (lhs.type & likely_signed) ? Intrinsic::ssub_with_overflow : Intrinsic::usub_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = (lhs.type & likely_signed) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, builder.zero(lhs)), builder.intMax(lhs), builder.intMin(lhs)) : builder.intMin(lhs).value;
                return new likely_expression(LikelyValue(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), type));
            } else {
                return new likely_expression(LikelyValue(builder.CreateSub(lhs, rhs), type));
            }
        }
    }
};
LIKELY_REGISTER(subtract)

class multiplyExpression : public SimpleArithmeticOperator
{
    const char *symbol() const { return "*"; }
    Value *evaluateSimpleArithmetic(Builder &builder, const LikelyValue &lhs, const LikelyValue &rhs) const
    {
        if (lhs & likely_floating) {
            return builder.CreateFMul(lhs, rhs);
        } else {
            if (lhs.type & likely_saturated) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module->module, (lhs.type & likely_signed) ? Intrinsic::smul_with_overflow : Intrinsic::umul_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *zero = builder.zero(lhs);
                Value *overflowResult = (lhs.type & likely_signed) ? builder.CreateSelect(builder.CreateXor(builder.CreateICmpSGE(lhs, zero), builder.CreateICmpSGE(rhs, zero)), builder.intMin(lhs), builder.intMax(lhs)) : builder.intMax(lhs).value;
                return builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0));
            } else {
                return builder.multiplyInts(lhs, rhs);
            }
        }
    }
};
LIKELY_REGISTER(multiply)

class divideExpression : public SimpleArithmeticOperator
{
    const char *symbol() const { return "/"; }
    Value *evaluateSimpleArithmetic(Builder &builder, const LikelyValue &n, const LikelyValue &d) const
    {
        if (n.type & likely_floating) {
            return builder.CreateFDiv(n, d);
        } else {
            if (n.type & likely_signed) {
                if (n.type & likely_saturated) {
                    Value *safe_i = builder.CreateAdd(n, builder.CreateZExt(builder.CreateICmpNE(builder.CreateOr(builder.CreateAdd(d, builder.one(n)), builder.CreateAdd(n, builder.intMin(n))), builder.zero(n)), n.value->getType()));
                    return builder.CreateSDiv(safe_i, d);
                } else {
                    return builder.CreateSDiv(n, d);
                }
            } else {
                return builder.CreateUDiv(n, d);
            }
        }
    }
};
LIKELY_REGISTER(divide)

class remainderExpression : public SimpleArithmeticOperator
{
    const char *symbol() const { return "%"; }
    Value *evaluateSimpleArithmetic(Builder &builder, const LikelyValue &lhs, const LikelyValue &rhs) const
    {
        return (lhs.type & likely_floating) ? builder.CreateFRem(lhs, rhs)
                                            : ((lhs.type & likely_signed) ? builder.CreateSRem(lhs, rhs)
                                                                          : builder.CreateURem(lhs, rhs));
    }
};
LIKELY_REGISTER(remainder)

#define LIKELY_REGISTER_LOGIC(OP, SYM)                                                                      \
class OP##Expression : public SimpleArithmeticOperator                                                      \
{                                                                                                           \
    const char *symbol() const { return #SYM; }                                                             \
    Value *evaluateSimpleArithmetic(Builder &builder, const LikelyValue &lhs, const LikelyValue &rhs) const \
    {                                                                                                       \
        return builder.Create##OP(lhs, rhs.value);                                                          \
    }                                                                                                       \
};                                                                                                          \
LIKELY_REGISTER(OP)                                                                                         \

LIKELY_REGISTER_LOGIC(And , &)
LIKELY_REGISTER_LOGIC(Or  , |)
LIKELY_REGISTER_LOGIC(Xor , ^)
LIKELY_REGISTER_LOGIC(Shl , <<)

class shiftRightExpression : public SimpleArithmeticOperator
{
    const char *symbol() const { return ">>"; }
    Value *evaluateSimpleArithmetic(Builder &builder, const LikelyValue &lhs, const LikelyValue &rhs) const
    {
        return (lhs.type & likely_signed) ? builder.CreateAShr(lhs, rhs.value) : builder.CreateLShr(lhs, rhs.value);
    }
};
LIKELY_REGISTER(shiftRight)

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                                                    \
class OP##Expression : public ArithmeticOperator                                                                                                               \
{                                                                                                                                                              \
    const char *symbol() const { return #SYM; }                                                                                                                \
    likely_const_expr evaluateArithmetic(Builder &builder, const LikelyValue &lhs, const LikelyValue &rhs) const                                               \
    {                                                                                                                                                          \
        return new likely_expression(builder.compare(lhs, rhs, Builder::OP));                                                                                  \
    }                                                                                                                                                          \
};                                                                                                                                                             \
LIKELY_REGISTER(OP)                                                                                                                                            \

LIKELY_REGISTER_COMPARISON(LT, <)
LIKELY_REGISTER_COMPARISON(LE, <=)
LIKELY_REGISTER_COMPARISON(GT, >)
LIKELY_REGISTER_COMPARISON(GE, >=)
LIKELY_REGISTER_COMPARISON(EQ, ==)
LIKELY_REGISTER_COMPARISON(NE, !=)

class BinaryMathOperator : public SimpleBinaryOperator
{
    likely_const_expr evaluateSimpleBinary(Builder &builder, const UniqueExpression &x, const UniqueExpression &n) const
    {
        const likely_type type = likely_type_from_types(likely_type_from_types(*x, *n), likely_floating);
        const likely_expression xc(builder.cast(*x.get(), type));
        const likely_expression nc(builder.cast(*n.get(), type));
        return new likely_expression(LikelyValue(builder.CreateCall2(Intrinsic::getDeclaration(builder.module->module, id(), xc.value->getType()), xc, nc), xc));
    }
    virtual Intrinsic::ID id() const = 0;
};

#define LIKELY_REGISTER_BINARY_MATH(OP)                \
class OP##Expression : public BinaryMathOperator       \
{                                                      \
    const char *symbol() const { return #OP; }         \
    Intrinsic::ID id() const { return Intrinsic::OP; } \
};                                                     \
LIKELY_REGISTER(OP)                                    \

LIKELY_REGISTER_BINARY_MATH(pow)
LIKELY_REGISTER_BINARY_MATH(copysign)

class tryExpression : public LikelyOperator
{
    const char *symbol() const { return "try"; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_expr value = get(builder, ast->atoms[1]);
        if (!value)
            value = get(builder, ast->atoms[2]);
        return value;
    }
};
LIKELY_REGISTER(try)

struct Lambda : public LikelyFunction
{
    const likely_const_ast body, parameters;

    Lambda(likely_const_env env, likely_const_ast body, likely_const_ast parameters = NULL)
        : LikelyFunction(env, length(parameters))
        , body(likely_retain_ast(body))
        , parameters(likely_retain_ast(parameters)) {}

    ~Lambda()
    {
        likely_release_ast(parameters);
        likely_release_ast(body);
    }

private:
    likely_const_expr evaluateFunction(Builder &builder, vector<likely_const_expr> &args) const
    {
        assert(args.size() == maxParameters());

        // Do dynamic dispatch if the type isn't fully specified
        bool dynamic = false;
        for (likely_const_expr arg : args)
            dynamic |= (arg->type == likely_multi_dimension);

        if (dynamic) {
            vector<likely_type> types;
            for (likely_const_expr arg : args)
                types.push_back(arg->type);

            const likely_vtable vtable = new likely_virtual_table(env, body, parameters, types);
            if (!env->expr)
                const_cast<likely_expr&>(env->expr) = new likely_expression();
            env->expr->vtables.push_back(vtable);

            PointerType *const vTableType = PointerType::getUnqual(StructType::create(builder.getContext(), "VTable"));
            StructType *const staticDataStructType = getStaticDataType(builder.module->context.get(), types);
            PointerType *const staticDataType = PointerType::getUnqual(Type::getInt8Ty(builder.getContext()));
            Function *likelyDynamic = builder.module->module->getFunction("likely_dynamic");
            if (!likelyDynamic) {
                Type *const params[] = { vTableType,
                                         PointerType::getUnqual(builder.module->context->toLLVM(likely_multi_dimension)),
                                         staticDataType };
                FunctionType *const likelyDynamicType = FunctionType::get(builder.module->context->toLLVM(likely_multi_dimension), params, false);
                likelyDynamic = Function::Create(likelyDynamicType, GlobalValue::ExternalLinkage, "likely_dynamic", builder.module->module);
                likelyDynamic->setCallingConv(CallingConv::C);
                likelyDynamic->setDoesNotAlias(0);
                likelyDynamic->setDoesNotAlias(1);
                likelyDynamic->setDoesNotCapture(1);
                likelyDynamic->setDoesNotAlias(2);
                likelyDynamic->setDoesNotCapture(2);
                sys::DynamicLibrary::AddSymbol("likely_dynamic", (void*) likely_dynamic);
            }

            const size_t dynamicArguments = args.size() - staticDataStructType->getNumElements();
            Value *const matricies = builder.CreateAlloca(builder.module->context->toLLVM(likely_multi_dimension), builder.constant(dynamicArguments));
            {
                size_t index = 0;
                for (size_t i=0; i<args.size(); i++)
                    if (types[i] == likely_multi_dimension)
                        builder.CreateStore(*args[i], builder.CreateGEP(matricies, builder.constant(index++)));
                assert(index == dynamicArguments);
            }

            Value *staticData;
            if (staticDataStructType->getNumElements() > 0) {
                staticData = builder.CreateAlloca(staticDataStructType, builder.one());
                size_t index = 0;
                for (size_t i=0; i<args.size(); i++)
                    if (types[i] != likely_multi_dimension)
                        builder.CreateStore(*args[i], builder.CreateStructGEP(staticData, static_cast<unsigned int>(index++)));
                assert(index == staticDataStructType->getNumElements());
                staticData = builder.CreatePointerCast(staticData, staticDataType);
            } else {
                staticData = ConstantPointerNull::get(staticDataType);
            }

            Value *const args[] = { ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(vtable)), uintptr_t(vtable)), vTableType),
                                    matricies,
                                    staticData };
            return new likely_expression(LikelyValue(builder.CreateCall(likelyDynamic, args), likely_multi_dimension));
        }

        likely_const_env restore = env;
        swap(builder.env, restore);

        if (parameters) {
            if (parameters->type == likely_ast_list) {
                for (size_t i=0; i<args.size(); i++)
                    define(builder.env, parameters->atoms[i]->atom, args[i]);
            } else {
                define(builder.env, parameters->atom, args[0]);
            }
        }

        const likely_const_expr result = get(builder, body);

        if (parameters)
            undefineAll(builder.env, parameters);
        args.clear();

        swap(builder.env, restore);
        return result;
    }
};

class lambdaExpression : public LikelyOperator
{
    const char *symbol() const { return "->"; }
    size_t maxParameters() const { return 2; }
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const { return new Lambda(builder.env, ast->atoms[2], ast->atoms[1]); }
};
LIKELY_REGISTER(lambda)

class externExpression : public LikelyOperator
{
    const char *symbol() const { return "extern"; }
    size_t maxParameters() const { return 5; }
    size_t minParameters() const { return 3; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        bool ok;
        const likely_type returnType = evalType(ast->atoms[1], builder.env, &ok);
        if (!ok)
            return NULL;

        const string name = evalString(ast->atoms[2], builder.env, &ok);
        if (!ok)
            return NULL;

        vector<likely_type> parameters;
        if (ast->atoms[3]->type == likely_ast_list) {
            for (uint32_t i=0; i<ast->atoms[3]->num_atoms; i++) {
                parameters.push_back(evalType(ast->atoms[3]->atoms[i], builder.env, &ok));
                if (!ok)
                    return NULL;
            }
        } else {
            parameters.push_back(evalType(ast->atoms[3], builder.env, &ok));
            if (!ok)
                return NULL;
        }

        if (ast->num_atoms < 5) {
            if (builder.module && builder.module->context->runtime_only) {
                const bool runtimeSymbol = (name == "likely_new");
                likely_ensure(runtimeSymbol, "referenced non-runtime symbol: %s", name.c_str());
            }
            return new Symbol(builder.env, name, returnType, parameters);
        }

        const LikelyFunction::CallingConvention cc = (ast->num_atoms >= 6) && (evalInt(ast->atoms[5], builder.env, &ok) != 0) ? LikelyFunction::ArrayCC : LikelyFunction::RegularCC;
        if (!ok)
            return NULL;

        if (builder.module /* static compilation */) {
            const UniqueExpression function(likely_expression::get(builder, ast->atoms[4]));
            const UniqueExpression f(reinterpret_cast<const LikelyFunction*>(function.get())->generate(builder, parameters, name, false, cc));
            if (f)
                return new Symbol(builder.env, name, f->type, parameters);
        } else /* JIT compilation */ {
            JITFunction *const jitFunction = new JITFunction(name, ast->atoms[4], builder.env, parameters, cc);
            if (jitFunction->function) {
                sys::DynamicLibrary::AddSymbol(name, jitFunction->function);
                return jitFunction;
            } else {
                release(jitFunction);
            }
        }
        return NULL;
    }
};
LIKELY_REGISTER(extern)

class defineExpression : public LikelyOperator
{
    const char *symbol() const { return "="; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const likely_const_ast rhs = ast->atoms[2];
        const char *const name = likely_symbol(ast);
        likely_const_expr expr = get(builder, rhs);
        define(builder.env, name, expr);
        return new likely_expression();
    }
};
LIKELY_REGISTER(define)

struct Assignable : public likely_expression
{
    Assignable(Value *value, likely_type type)
    {
        this->value = value;
        this->type = type;
    }

    virtual void set(Builder &builder, const likely_expression &expr, likely_const_ast ast) const = 0;

    static const Assignable *dynamicCast(likely_const_expr expr)
    {
        return (expr && (expr->uid() == UID())) ? static_cast<const Assignable*>(expr) : NULL;
    }

private:
    static int UID() { return __LINE__; }
    int uid() const { return UID(); }
    size_t maxParameters() const { return 4; }
    size_t minParameters() const { return 0; }
};

struct Variable : public Assignable
{
    Variable(Builder &builder, const LikelyValue &expr)
        : Assignable(builder.CreateAlloca(builder.module->context->toLLVM(expr), 0), likely_pointer_type(expr))
    {
        builder.CreateStore(expr, value);
    }

    Variable(Builder &builder, const LikelyValue &expr, const LikelyValue &size)
        : Assignable(builder.CreateAlloca(builder.module->context->toLLVM(expr), size), likely_pointer_type(expr))
    {
        // Create a loop to initialize the entire array
        BasicBlock *const entry = builder.GetInsertBlock();
        BasicBlock *const body = BasicBlock::Create(builder.getContext(), "variable_init_body", entry->getParent());
        BasicBlock *const exit = BasicBlock::Create(builder.getContext(), "variable_init_exit", body->getParent());
        builder.CreateBr(body);
        builder.SetInsertPoint(body);
        PHINode *const phiNode = builder.CreatePHI(Type::getInt32Ty(builder.getContext()), 2);
        phiNode->addIncoming(builder.zero(likely_u32), entry);
        builder.CreateStore(expr, builder.CreateGEP(value, phiNode));
        Value *const increment = builder.addInts(phiNode, builder.one(likely_u32));
        Value *const postcondition = builder.CreateICmpNE(increment, size);
        builder.CreateCondBr(postcondition, body, exit);
        phiNode->addIncoming(increment, body);
        builder.SetInsertPoint(exit);
    }

private:
    void set(Builder &builder, const likely_expression &expr, likely_const_ast ast) const
    {
        Value *ptr;
        if (ast->type == likely_ast_list) {
            // array
            assert(ast->num_atoms == 2);
            const UniqueExpression index(get(builder, ast->atoms[1]));
            assert(index);
            ptr = builder.CreateGEP(value, builder.cast(*index, likely_u32));
        } else {
            // scalar
            ptr = value;
        }
        builder.CreateStore(builder.cast(expr, likely_element_type(type)), ptr);
    }

    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        Value *ptr;
        if (ast->type == likely_ast_list) {
            assert(ast->num_atoms == 2);
            const UniqueExpression index(get(builder, ast->atoms[1]));
            assert(index);
            ptr = builder.CreateGEP(value, builder.cast(*index, likely_u32));
        } else {
            // scalar
            ptr = value;
        }
        return new likely_expression(LikelyValue(builder.CreateLoad(ptr), likely_element_type(type)));
    }
};

class allocateExpression : public LikelyOperator
{
    const char *symbol() const { return "$"; }
    size_t minParameters() const { return 1; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[1], value);
        if (ast->num_atoms == 2) {
            // scalar
            return new Variable(builder, *value);
        } else {
            // array
            TRY_EXPR(builder, ast->atoms[1], size);
            return new Variable(builder, *value, builder.cast(*size, likely_u32));
        }
    }
};
LIKELY_REGISTER(allocate)

class storeExpression : public LikelyOperator
{
    const char *symbol() const { return "<-"; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[2], expr);
        const likely_const_env env = lookup(builder.env, likely_symbol(ast->atoms[1]));
        assert(env);
        const likely_const_expr pointer = env->expr;
        assert(pointer);

        if (const Assignable *const assignable = Assignable::dynamicCast(pointer))
            assignable->set(builder, *expr, ast->atoms[1]);
        else if (pointer->type & likely_compound_pointer)
            builder.CreateStore(builder.cast(*expr, likely_element_type(pointer->type)), pointer->value);
        else
            assert(false);

        return new likely_expression();
    }
};
LIKELY_REGISTER(store)

class assumeExpression : public SimpleUnaryOperator
{
    const char *symbol() const { return "assume"; }
    likely_const_expr evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        builder.assume(*arg);
        return new likely_expression();
    }
};
LIKELY_REGISTER(assume)

class beginExpression : public LikelyOperator
{
    const char *symbol() const { return "{"; }
    size_t minParameters() const { return 2; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_expr result = NULL;
        likely_const_env root = builder.env;
        for (size_t i=1; i<ast->num_atoms-2; i++) {
            const UniqueExpression expr(get(builder, ast->atoms[i]));
            if (!expr.get())
                goto cleanup;
        }
        result = get(builder, ast->atoms[ast->num_atoms-2]);
        // the last operand is "}"

    cleanup:
        while (builder.env != root) {
            likely_const_env old = builder.env;
            builder.env = builder.env->parent;
            likely_release_env(old);
        }
        return result;
    }
};
LIKELY_REGISTER(begin)

class evaluateExpression : public LikelyOperator
{
    const char *symbol() const { return "["; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const Variant data = Lambda(builder.env, ast->atoms[1]).evaluateConstantFunction();
        return data ? new ConstantData(builder, data) : NULL;
    }
};
LIKELY_REGISTER(evaluate)

class ifExpression : public LikelyOperator
{
    const char *symbol() const { return "?"; }
    size_t minParameters() const { return 2; }
    size_t maxParameters() const { return 3; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        Function *function = builder.GetInsertBlock()->getParent();
        const bool hasElse = ast->num_atoms == 4;

        TRY_EXPR(builder, ast->atoms[1], Cond)

        // Special case where the conditional is a constant value
        if (ConstantInt *const constantInt = dyn_cast<ConstantInt>(Cond->value)) {
            if (constantInt->isZero()) {
                if (hasElse) return get(builder, ast->atoms[3]);
                else         return new likely_expression();
            } else {
                return get(builder, ast->atoms[2]);
            }
        }

        BasicBlock *const Entry = builder.GetInsertBlock();
        BasicBlock *const True = BasicBlock::Create(builder.getContext(), "then", function);
        BasicBlock *const False = hasElse ? BasicBlock::Create(builder.getContext(), "else", function) : NULL;
        BasicBlock *const End = BasicBlock::Create(builder.getContext(), "end", function);
        Instruction *const condBr = builder.CreateCondBr(*Cond, True, hasElse ? False : End);

        builder.SetInsertPoint(True);
        TRY_EXPR(builder, ast->atoms[2], t)

        if (hasElse) {
            builder.SetInsertPoint(False);
            TRY_EXPR(builder, ast->atoms[3], f)
            const likely_type resolved = likely_type_from_types(*t, *f);

            builder.SetInsertPoint(True);
            const likely_expression tc(builder.cast(*t, resolved));
            builder.CreateBr(End);

            builder.SetInsertPoint(False);
            const likely_expression fc(builder.cast(*f, resolved));
            builder.CreateBr(End);

            builder.SetInsertPoint(End);
            if ((True->size() == 1) && (False->size() == 1)) {
                // Special case where the conditional is reducible to a select instruction
                condBr->eraseFromParent();
                True->eraseFromParent();
                False->eraseFromParent();
                End->eraseFromParent();
                builder.SetInsertPoint(Entry);
                return new likely_expression(LikelyValue(builder.CreateSelect(*Cond, tc, fc), resolved));
            } else {
                // General case
                PHINode *const phi = builder.CreatePHI(builder.module->context->toLLVM(resolved), 2);
                phi->addIncoming(tc, True);
                phi->addIncoming(fc, False);
                return new likely_expression(LikelyValue(phi, resolved));
            }
        } else {
            if (True->empty() || !True->back().isTerminator())
                builder.CreateBr(End);
            builder.SetInsertPoint(End);
            return new likely_expression();
        }
    }
};
LIKELY_REGISTER(if)

struct Label : public likely_expression
{
    Label(BasicBlock *basicBlock)
        : likely_expression(LikelyValue(basicBlock, likely_void)) {}

private:
    likely_const_expr evaluate(Builder &builder, likely_const_ast) const
    {
        BasicBlock *basicBlock = cast<BasicBlock>(value);
        builder.CreateBr(basicBlock);
        return new Label(basicBlock);
    }
};

class labelExpression : public Operand
{
    const char *symbol() const { return "#"; }
    likely_const_expr evaluateOperand(Builder &builder) const
    {
        BasicBlock *label = BasicBlock::Create(builder.getContext(), "label", builder.GetInsertBlock()->getParent());
        builder.CreateBr(label);
        builder.SetInsertPoint(label);
        return new Label(label);
    }
};
LIKELY_REGISTER(label)

class kernelExpression : public LikelyOperator
{
    struct KernelArgument : public Assignable
    {
        const string name;
        MDNode *node = NULL;
        Value *channels, *columns, *rows, *frames;
        Value *rowStep, *frameStep;
        Value *data;

        KernelArgument(Builder &builder, const likely_expression &matrix, const string &name)
            : Assignable(matrix.value, matrix.type), name(name)
        {
            channels   = builder.cast(builder.channels(*this), likely_u64);
            columns    = builder.cast(builder.columns (*this), likely_u64);
            rows       = builder.cast(builder.rows    (*this), likely_u64);
            frames     = builder.cast(builder.frames  (*this), likely_u64);
            rowStep    = builder.multiplyInts(channels, columns);
            frameStep  = builder.multiplyInts(rowStep, rows);
            data       = builder.data(*this);
            channels ->setName(name + "_c");
            columns  ->setName(name + "_x");
            rows     ->setName(name + "_y");
            frames   ->setName(name + "_t");
            rowStep  ->setName(name + "_y_step");
            frameStep->setName(name + "_t_step");

            if (data) {
                // This is how we communicate data alignment guarantee
                Value *const ptrint = builder.CreatePtrToInt(data, Type::getInt64Ty(builder.getContext()));
                Value *const maskedptr = builder.CreateAnd(ptrint, 31);
                Value *const maskcond = builder.CreateICmpEQ(maskedptr, builder.zero());
                builder.assume(maskcond);
            }
        }

    private:
        Value *gep(Builder &builder, likely_const_ast ast) const
        {
            const size_t len = length(ast);
            Value *i = builder.zero();
            if (type & likely_multi_channel) {
                static const likely_const_ast defaultC = likely_atom("c", 1);
                Value *const c = builder.cast(*UniqueExpression(get(builder, (len >= 2) ? ast->atoms[1] : defaultC)), likely_u64).value;
                i = builder.addInts(c, i);
            }
            if (type & likely_multi_column) {
                static const likely_const_ast defaultX = likely_atom("x", 1);
                Value *const x = builder.cast(*UniqueExpression(get(builder, (len >= 3) ? ast->atoms[2] : defaultX)), likely_u64).value;
                i = builder.addInts(builder.multiplyInts(x, channels), i);
            }
            if (type & likely_multi_row) {
                static const likely_const_ast defaultY = likely_atom("y", 1);
                Value *const y = builder.cast(*UniqueExpression(get(builder, (len >= 4) ? ast->atoms[3] : defaultY)), likely_u64).value;
                i = builder.addInts(builder.multiplyInts(y, rowStep), i);
            }
            if (type & likely_multi_frame) {
                static const likely_const_ast defaultT = likely_atom("t", 1);
                Value *const t = builder.cast(*UniqueExpression(get(builder, (len >= 5) ? ast->atoms[4] : defaultT)), likely_u64).value;
                i = builder.addInts(builder.multiplyInts(t, frameStep), i);
            }
            return builder.CreateGEP(data, i);
        }

        void set(Builder &builder, const likely_expression &expr, likely_const_ast ast) const
        {
            StoreInst *const store = builder.CreateStore(builder.cast(expr, type & likely_element), gep(builder, ast));
            if (node)
                store->setMetadata("llvm.mem.parallel_loop_access", node);
        }

        likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
        {
            if (!isa<PointerType>(value->getType()))
                return new likely_expression((LikelyValue) *this);

            LoadInst *const load = builder.CreateLoad(gep(builder, ast));
            if (node)
                load->setMetadata("llvm.mem.parallel_loop_access", node);
            return new likely_expression(LikelyValue(load, type & likely_element));
        }
    };

    struct KernelAxis : public likely_expression
    {
        string name;
        Value *start, *stop, *increment, *postcondition;
        BasicBlock *entry, *body, *exit;
        BranchInst *latch;
        KernelAxis *parent, *child;

        KernelAxis(Builder &builder, const string &name, Value *start, Value *stop, KernelAxis *parent)
            : name(name), start(start), stop(stop), exit(NULL), latch(NULL), parent(parent), child(NULL)
        {
            entry = builder.GetInsertBlock();
            body = BasicBlock::Create(builder.getContext(), name + "_body", entry->getParent());
            exit = BasicBlock::Create(builder.getContext(), name + "_exit", body->getParent());
            builder.CreateBr(body);
            builder.SetInsertPoint(body);
            type = toLikely(start->getType());
            value = builder.CreatePHI(builder.module->context->toLLVM(type), 2, name);
            cast<PHINode>(value)->addIncoming(start, entry);

            if (parent)
                parent->child = this;
        }

        void close(Builder &builder)
        {
            increment = builder.addInts(value, builder.one(type), name + "_increment");
            postcondition = builder.CreateICmpNE(increment, stop, name + "_postcondition");
            latch = builder.CreateCondBr(postcondition, body, exit);
            cast<PHINode>(value)->addIncoming(increment, builder.GetInsertBlock());
            builder.SetInsertPoint(exit);

            if (child) exit->moveAfter(child->exit);
            if (parent) parent->close(builder);
        }

        void tryCollapse(Builder &builder, MDNode *node)
        {
            bool collapsible = !!child; // To be collapsible there must be a child loop to collapse

            // To be collapsible we must be able to replace all the uses of our value
            set<Value*> replaceables;
            if (collapsible)
                for (User *const user : value->users()) {
                    if (user == increment)
                        continue; // Trivially replaceable

                    // We can only replace (+ (* value child->stop) child->value)
                    if (MulOperator *const mulOperator = dyn_cast<MulOperator>(user))
                        if (mulOperator->getOperand(0) == value) {
                            Instruction *const a = dyn_cast<Instruction>(mulOperator->getOperand(1));
                            Instruction *const b = dyn_cast<Instruction>(child->stop);
                            if (a && b && a->isSameOperationAs(b)) { // Match (* value child->stop)
                                for (const Use &use : mulOperator->uses()) {
                                    if (AddOperator *const addOperator = dyn_cast<AddOperator>(use.getUser())) // Match (+ (* value child->stop) child->value)
                                        if ((addOperator->getOperand(0) == mulOperator) && (addOperator->getOperand(1) == child->value)) {
                                            replaceables.insert(addOperator);
                                            continue;
                                        }

                                    collapsible = false;
                                    break;
                                }
                                continue;
                            }
                        }

                    collapsible = false;
                    break;
                }

            // To be collapsible all of our childs uses must have been identified for replacement
            if (collapsible)
                for (User *const user : child->value->users()) {
                    if (user == child->increment)
                        continue; // Trivially replaceable

                    if (replaceables.find(user) != replaceables.end())
                        continue; // We know that it is replaceable

                    collapsible = false;
                    break;
                }

            if (collapsible) {
                // Update our range
                BasicBlock *const restore = builder.GetInsertBlock();
                const KernelAxis *ancestor = this;
                while (ancestor->parent)
                    ancestor = ancestor->parent;
                builder.SetInsertPoint(cast<Instruction>(ancestor->entry->getTerminator())); // At this point we know that all of the `start` and `stop` values have been created but not yet used
                Value *const newStart = builder.multiplyInts(start, child->stop);
                Value *const newStop = builder.multiplyInts(stop, child->stop);
                cast<PHINode>(value)->setIncomingValue(0, newStart);
                cast<ICmpInst>(postcondition)->setOperand(1, newStop);
                start = newStart;
                stop = newStop;

                // Update our replaceables
                for (Value *const replaceable : replaceables)
                    replaceable->replaceAllUsesWith(value);

                // Collapse the child loop
                child->value->replaceAllUsesWith(builder.zero());
                cast<Instruction>(child->value)->eraseFromParent();
                builder.SetInsertPoint(child->latch->getParent());
                BranchInst *const newChildLatch = builder.CreateBr(child->exit);
                child->latch->eraseFromParent();
                child->latch = newChildLatch;
                MergeBlockIntoPredecessor(child->exit);
                MergeBlockIntoPredecessor(child->body);
                child->exit = NULL;
                child->body = NULL;
                builder.SetInsertPoint(restore);

                // Remove dead instructions to facilitate collapsing additional loops
                DCE(*restore->getParent());
            } else if (child) {
                // We couldn't collapse the child loop, mark it for vectorization
                child->latch->setMetadata("llvm.loop", node);
                node = NULL;
            }

            if (node) {
                if (parent) parent->tryCollapse(builder, node);    // Continue collapsing loops
                else        latch->setMetadata("llvm.loop", node); // There is no child loop to collapse, mark us for vectorization
            }
        }
    };

    const char *symbol() const { return "=>"; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        vector<likely_const_expr> srcs;
        const likely_const_ast args = ast->atoms[1];
        const size_t argsStart = ((args->type == likely_ast_list) && (args->atoms[0]->type == likely_ast_list)) ? 1 : 0;
        const uint32_t manualDims = argsStart ? args->atoms[0]->num_atoms : 0;
        if (args->type == likely_ast_list) {
            for (size_t j=argsStart; j<args->num_atoms; j++)
                srcs.push_back(get(builder, args->atoms[j]));
            for (size_t j=0; j<manualDims; j++) {
                const UniqueExpression expr(get(builder, args->atoms[0]->atoms[j]));
                srcs.push_back(new likely_expression(builder.cast(*expr, likely_u64)));
            }
        } else {
            srcs.push_back(get(builder, args));
        }

        likely_type kernelType = likely_void;
        if (argsStart) {
            for (uint32_t i=0; i<manualDims; i++) {
                if (IsOne(srcs[srcs.size() - manualDims + i]->value))
                    continue;
                if      (i == 0) kernelType |= likely_multi_channel;
                else if (i == 1) kernelType |= likely_multi_column;
                else if (i == 2) kernelType |= likely_multi_row;
                else if (i == 3) kernelType |= likely_multi_frame;
            }
        } else {
            kernelType = srcs[0]->type;
        }

        Value *kernelSize;
        if      (kernelType & likely_multi_frame)   kernelSize = argsStart ? srcs[srcs.size() - manualDims + 3]->value : builder.cast(builder.frames  (*srcs[0]), likely_u64).value;
        else if (kernelType & likely_multi_row)     kernelSize = argsStart ? srcs[srcs.size() - manualDims + 2]->value : builder.cast(builder.rows    (*srcs[0]), likely_u64).value;
        else if (kernelType & likely_multi_column)  kernelSize = argsStart ? srcs[srcs.size() - manualDims + 1]->value : builder.cast(builder.columns (*srcs[0]), likely_u64).value;
        else if (kernelType & likely_multi_channel) kernelSize = argsStart ? srcs[srcs.size() - manualDims + 0]->value : builder.cast(builder.channels(*srcs[0]), likely_u64).value;
        else                                        kernelSize = builder.one();

        const bool serial = isa<ConstantInt>(kernelSize) && (IsOne(kernelSize));
        if      (builder.module->context->heterogeneous && !serial) generateHeterogeneous(builder, ast, srcs, kernelType, kernelSize);
        else if (builder.module->context->multicore     && !serial) generateMulticore    (builder, ast, srcs, kernelType, kernelSize);
        else                                                        generateSerial       (builder, ast, srcs, kernelType, kernelSize);

        for (size_t i=1; i<srcs.size(); i++)
            release(srcs[i]);
        return srcs[0];
    }

    void generateSerial(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, likely_type kernelType, Value *kernelSize) const
    {
        generateCommon(builder, ast, srcs, kernelType, builder.zero(), kernelSize);
    }

    void generateMulticore(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, likely_type kernelType, Value *kernelSize) const
    {
        BasicBlock *entry = builder.GetInsertBlock();

        vector<Type*> parameterTypes;
        for (const likely_const_expr src : srcs)
            parameterTypes.push_back(src->value->getType());
        StructType *const parameterStructType = StructType::get(builder.getContext(), parameterTypes);

        Function *thunk;
        {
            Type *const params[] = { PointerType::getUnqual(parameterStructType), builder.module->context->toLLVM(likely_u64), builder.module->context->toLLVM(likely_u64) };
            FunctionType *const thunkType = FunctionType::get(Type::getVoidTy(builder.getContext()), params, false);

            // Functions can have multiple kernels, start from 0 until an unused thunk name is found
            string name;
            int i = 0;
            while (name.empty() || builder.module->module->getFunction(name)) {
                stringstream stream;
                stream << builder.GetInsertBlock()->getParent()->getName().str() << "_thunk" << i;
                name = stream.str();
                i++;
            }

            thunk = cast<Function>(builder.module->module->getOrInsertFunction(name, thunkType));
            thunk->addFnAttr(Attribute::NoUnwind);
            thunk->setCallingConv(CallingConv::C);
            thunk->setLinkage(GlobalValue::PrivateLinkage);
            thunk->setDoesNotAlias(1);
            thunk->setDoesNotCapture(1);

            Function::arg_iterator it = thunk->arg_begin();
            Value *const parameterStruct = it++;
            Value *const start = it++;
            Value *const stop = it++;

            builder.SetInsertPoint(BasicBlock::Create(builder.getContext(), "entry", thunk));
            vector<likely_const_expr> thunkSrcs;
            for (size_t i=0; i<srcs.size(); i++)
                thunkSrcs.push_back(new likely_expression(LikelyValue(builder.CreateLoad(builder.CreateStructGEP(parameterStruct, unsigned(i))), srcs[i]->type)));

            generateCommon(builder, ast, thunkSrcs, kernelType, start, stop);
            const_cast<likely_expr>(srcs[0])->type = thunkSrcs[0]->type;
            for (likely_const_expr thunkSrc : thunkSrcs)
                release(thunkSrc);
            builder.CreateRetVoid();
        }

        builder.SetInsertPoint(entry);

        Type *const voidPtr = Type::getInt8PtrTy(builder.getContext());
        Type *const params[] = { voidPtr, voidPtr, builder.module->context->toLLVM(likely_u64) };
        FunctionType *const likelyForkType = FunctionType::get(Type::getVoidTy(builder.getContext()), params, false);
        Function *likelyFork = builder.module->module->getFunction("likely_fork");
        if (!likelyFork) {
            likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "likely_fork", builder.module->module);
            likelyFork->setCallingConv(CallingConv::C);
            likelyFork->setDoesNotCapture(1);
            likelyFork->setDoesNotAlias(1);
            likelyFork->setDoesNotCapture(2);
            likelyFork->setDoesNotAlias(2);
            sys::DynamicLibrary::AddSymbol("likely_fork", (void*) likely_fork);
        }

        Value *const parameterStruct = builder.CreateAlloca(parameterStructType);
        for (size_t i=0; i<srcs.size(); i++)
            builder.CreateStore(*srcs[i], builder.CreateStructGEP(parameterStruct, unsigned(i)));

        builder.CreateCall3(likelyFork, builder.CreatePointerCast(builder.module->module->getFunction(thunk->getName()), voidPtr), builder.CreatePointerCast(parameterStruct, voidPtr), kernelSize);
    }

    void generateHeterogeneous(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, likely_type kernelType, Value *kernelSize) const
    {
        likely_initialize_coprocessor();

        likely_module kernelModule(*builder.env->settings);
        kernelModule.module->setTargetTriple("nvptx-nvidia-cuda");
        NamedMDNode *const nvvmAnnotations = kernelModule.module->getOrInsertNamedMetadata("nvvm.annotations");
        Builder kernelBuilder(builder.env, &kernelModule);

        vector<Type*> parameterTypes;
        for (const likely_const_expr src : srcs)
            parameterTypes.push_back(src->value->getType());
        FunctionType *const kernelFunctionType = FunctionType::get(Type::getVoidTy(kernelBuilder.getContext()), parameterTypes, false);
        Function *const kernel = cast<Function>(kernelBuilder.module->module->getOrInsertFunction(builder.GetInsertBlock()->getParent()->getName().str() + "_kernel", kernelFunctionType));
        Metadata* metadata[3] = { ValueAsMetadata::get(kernel),
                                  MDString::get(kernelBuilder.getContext(), "kernel"),
                                  ValueAsMetadata::get(kernelBuilder.one(likely_u32)) };
        nvvmAnnotations->addOperand(MDTuple::get(kernelBuilder.getContext(), metadata));

        kernelBuilder.SetInsertPoint(BasicBlock::Create(kernelBuilder.getContext(), "entry", kernel));

        Value *const c = (kernelType & likely_multi_channel) ? kernelBuilder.CreateCall(Intrinsic::getDeclaration(kernelBuilder.module->module, Intrinsic::ptx_read_tid_w, Type::getInt32Ty(kernelBuilder.getContext())), "c") : builder.one(likely_u32).value;
        Value *const x = (kernelType & likely_multi_column ) ? kernelBuilder.CreateCall(Intrinsic::getDeclaration(kernelBuilder.module->module, Intrinsic::ptx_read_tid_x, Type::getInt32Ty(kernelBuilder.getContext())), "x") : builder.one(likely_u32).value;
        Value *const y = (kernelType & likely_multi_row    ) ? kernelBuilder.CreateCall(Intrinsic::getDeclaration(kernelBuilder.module->module, Intrinsic::ptx_read_tid_y, Type::getInt32Ty(kernelBuilder.getContext())), "y") : builder.one(likely_u32).value;
        Value *const t = (kernelType & likely_multi_frame  ) ? kernelBuilder.CreateCall(Intrinsic::getDeclaration(kernelBuilder.module->module, Intrinsic::ptx_read_tid_z, Type::getInt32Ty(kernelBuilder.getContext())), "t") : builder.one(likely_u32).value;

        define(kernelBuilder.env, "c", new likely_expression(LikelyValue(c, likely_u32)));
        define(kernelBuilder.env, "x", new likely_expression(LikelyValue(x, likely_u32)));
        define(kernelBuilder.env, "y", new likely_expression(LikelyValue(y, likely_u32)));
        define(kernelBuilder.env, "t", new likely_expression(LikelyValue(t, likely_u32)));

        vector<KernelArgument*> kernelArguments;
        Function::arg_iterator it = kernel->arg_begin();
        const likely_const_ast args = ast->atoms[1];
        const size_t argsStart = ((args->type == likely_ast_list) && (args->atoms[0]->type == likely_ast_list)) ? 1 : 0;
        if (args->type == likely_ast_list) {
            for (size_t i=argsStart; i<args->num_atoms; i++)
                kernelArguments.push_back(new KernelArgument(kernelBuilder, LikelyValue(it++, *srcs[i-argsStart]), args->atoms[i]->atom));
        } else {
            kernelArguments.push_back(new KernelArgument(kernelBuilder, LikelyValue(it++, *srcs[0]), args->atom));
        }

        for (KernelArgument *kernelArgument : kernelArguments) {
//            kernelArgument->node = node;
            define(kernelBuilder.env, kernelArgument->name.c_str(), kernelArgument);
        }

        undefineAll(kernelBuilder.env, args, argsStart);
        kernelArguments.clear();

        undefine(kernelBuilder.env, "t");
        undefine(kernelBuilder.env, "y");
        undefine(kernelBuilder.env, "x");
        undefine(kernelBuilder.env, "c");

        kernelBuilder.module->module->dump();

        // The heterogeneous backend isn't done yet so generate CPU code for the time being
        if (builder.module->context->multicore) generateMulticore(builder, ast, srcs, kernelType, kernelSize);
        else                                    generateSerial   (builder, ast, srcs, kernelType, kernelSize);
    }

    void generateCommon(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, likely_type kernelType, Value *start, Value *stop) const
    {
        vector<KernelArgument*> kernelArguments;
        const likely_const_ast args = ast->atoms[1];
        const size_t argsStart = ((args->type == likely_ast_list) && (args->atoms[0]->type == likely_ast_list)) ? 1 : 0;
        const uint32_t manualDims = argsStart ? args->atoms[0]->num_atoms : 0;
        assert(manualDims <= 4);
        if (args->type == likely_ast_list) {
            for (size_t i=argsStart; i<args->num_atoms; i++)
                kernelArguments.push_back(new KernelArgument(builder, *srcs[i-argsStart], args->atoms[i]->atom));
        } else {
            kernelArguments.push_back(new KernelArgument(builder, *srcs[0], args->atom));
        }

        Value *const manualChannels = (manualDims >= 1) ? srcs[srcs.size() - manualDims + 0]->value : NULL;
        Value *const manualColumns  = (manualDims >= 2) ? srcs[srcs.size() - manualDims + 1]->value : NULL;
        Value *const manualRows     = (manualDims >= 3) ? srcs[srcs.size() - manualDims + 2]->value : NULL;

        MDNode *node;
        { // Create self-referencing loop node
            Metadata *const Args[] = { 0 };
            node = MDNode::get(builder.getContext(), Args);
            node->replaceOperandWith(0, node);
        }

        KernelAxis *axis = NULL;
        if (kernelType & likely_multi_frame) {
            axis = new KernelAxis(builder, "t", start, stop, NULL);
            define(builder.env, "t", axis);
        } else {
            define(builder.env, "t", new likely_expression(builder.zero()));
        }

        if (kernelType & likely_multi_row) {
            axis = new KernelAxis(builder, "y", axis ? builder.zero().value : start
                                              , axis ? (argsStart ? manualRows : kernelArguments[0]->rows) : stop
                                              , axis);
            define(builder.env, "y", axis);
        } else {
            define(builder.env, "y", new likely_expression(builder.zero()));
        }

        if (kernelType & likely_multi_column) {
            axis = new KernelAxis(builder, "x", axis ? builder.zero().value : start
                                              , axis ? (argsStart ? manualColumns : kernelArguments[0]->columns) : stop
                                              , axis);
            define(builder.env, "x", axis);
        } else {
            define(builder.env, "x", new likely_expression(builder.zero()));
        }

        if (kernelType & likely_multi_channel) {
            axis = new KernelAxis(builder, "c", axis ? builder.zero().value : start
                                              , axis ? (argsStart ? manualChannels : kernelArguments[0]->channels) : stop
                                              , axis);
            define(builder.env, "c", axis);
        } else {
            define(builder.env, "c", new likely_expression(builder.zero()));
        }

        for (KernelArgument *const kernelArgument : kernelArguments) {
            kernelArgument->node = node;
            define(builder.env, kernelArgument->name.c_str(), kernelArgument);
        }

        release(get(builder, ast->atoms[2]));

        undefineAll(builder.env, args, argsStart);
        kernelArguments.clear();

        if (axis) {
            axis->exit->moveAfter(builder.GetInsertBlock());
            axis->close(builder);
        }

        // Clean up any instructions we didn't end up using
        DCE(*builder.GetInsertBlock()->getParent());

        if (axis && !ExperimentalLoopCollapse)
            axis->tryCollapse(builder, node);
        undefine(builder.env, "c");
        undefine(builder.env, "x");
        undefine(builder.env, "y");
        undefine(builder.env, "t");
    }
};
LIKELY_REGISTER(kernel)

} // namespace (anonymous)

likely_expression::~likely_expression()
{
    for (likely_vtable vtable : vtables)
        delete vtable;
}

likely_const_expr likely_expression::evaluate(Builder &builder, likely_const_ast ast) const
{
    if ((type & likely_compound_pointer) && (ast->type == likely_ast_list)) {
        const likely_type elementType = likely_element_type(type);
        if (ast->num_atoms == 1) {
            return new likely_expression(LikelyValue(builder.CreateLoad(value), elementType));
        } else if (ast->num_atoms == 2) {
            TRY_EXPR(builder, ast->atoms[1], arg);
            return new likely_expression(LikelyValue(builder.CreateLoad(builder.CreateGEP(value, *arg)), elementType));
        } else {
            return NULL;
        }
    }

    if ((type & likely_multi_dimension) && (ast->type == likely_ast_list)) {
        Value *const channel = (ast->num_atoms >= 2) ? builder.cast(*UniqueExpression(get(builder, ast->atoms[1])), likely_u64).value
                                                     : builder.zero().value;
        Value *const column  = (ast->num_atoms >= 3) ? builder.cast(*UniqueExpression(get(builder, ast->atoms[2])), likely_u64).value
                                                     : builder.zero().value;
        Value *const row     = (ast->num_atoms >= 4) ? builder.cast(*UniqueExpression(get(builder, ast->atoms[3])), likely_u64).value
                                                     : builder.zero().value;
        Value *const frame   = (ast->num_atoms >= 5) ? builder.cast(*UniqueExpression(get(builder, ast->atoms[4])), likely_u64).value
                                                     : builder.zero().value;

        Value *const channels = builder.cast(builder.channels(*this), likely_u64);
        Value *const columns  = builder.cast(builder.columns (*this), likely_u64);
        Value *const rows     = builder.cast(builder.rows    (*this), likely_u64);

        Value *const rowStep   = builder.multiplyInts(channels, columns);
        Value *const frameStep = builder.multiplyInts(rowStep, rows);

        Value *index = builder.CreateMul(frame, frameStep);
        index = builder.CreateAdd(index, builder.CreateMul(row, rowStep));
        index = builder.CreateAdd(index, builder.CreateMul(column, channels));
        index = builder.CreateAdd(index, channel);

        Value *const load = builder.CreateLoad(builder.CreateGEP(builder.data(*this), index));
        return new likely_expression(LikelyValue(load, type & likely_element));
    }

    return new likely_expression(LikelyValue(value, type));
}

likely_const_expr likely_expression::get(Builder &builder, likely_const_ast ast)
{
    likely_const_mat str = NULL;
    if (builder.env->settings->verbose) {
        str = likely_ast_to_string(ast, 2);
        outs() << "ENTERING: " << str->data << '\n';
    }

    const likely_const_expr result = _get(builder, ast);

    if (builder.env->settings->verbose) {
        outs() << "EXITING: " << str->data << '\n';
        likely_release_mat(str);

        outs() << "RESULT: ";
        if (result) {
            if (result->value)
                result->value->print(outs());
            else
                outs() << "<>";
        } else {
            outs() << "<error>";
        }
        outs() << '\n';
    }

    return result;
}

// As a special exception, this function is allowed to set ast->type
likely_const_expr likely_expression::_get(Builder &builder, likely_const_ast ast)
{
    if (ast->type == likely_ast_list) {
        if (ast->num_atoms == 0)
            return new likely_expression(); // Special case, return a void expression
        const likely_const_ast op = ast->atoms[0];

        if (op->type != likely_ast_list)
            if (const likely_const_env env = lookup(builder.env, op->atom)) {
                const_cast<likely_ast>(op)->type = likely_ast_operator;
                return env->expr->evaluate(builder, ast); // Environment variable _operators_ need not be lowered
            }

        TRY_EXPR(builder, op, expr);
        return expr->evaluate(builder, ast);
    } else {
        if (const likely_const_env env = lookup(builder.env, ast->atom)) {
            const_cast<likely_ast>(ast)->type = likely_ast_operand;
            return env->expr->evaluate(builder, ast); // Environment variable _operands_ need to be lowered
        }

        { // Is it an integer?
            char *p;
            const int64_t value = strtoll(ast->atom, &p, 0);
            if (*p == 0) {
                const_cast<likely_ast>(ast)->type = likely_ast_integer;
                return new likely_expression(builder.constant(uint64_t(value), int32_t(value) == value ? likely_i32 : likely_i64));
            }
        }

        { // Is it a real?
            char *p;
            const double value = strtod(ast->atom, &p);
            if (*p == 0) {
                const_cast<likely_ast>(ast)->type = likely_ast_real;
                return new likely_expression(builder.constant(value, float(value) == value ? likely_f32 : likely_f64));
            }
        }

        // Is it a string?
        if ((ast->atom[0] == '"') && (ast->atom[ast->atom_len-1] == '"')) {
            const_cast<likely_ast>(ast)->type = likely_ast_string;
            return new ConstantString(builder, likely_string(string(ast->atom).substr(1, ast->atom_len-2).c_str()));
        }

        { // Is it a matrix type?
            bool ok;
            const likely_type matrixType = likely_type_from_string(ast->atom, &ok);
            if (ok) {
                const_cast<likely_ast>(ast)->type = likely_ast_type;
                return new MatrixType(builder, matrixType);
            }
        }

        { // Is it a file type?
            bool ok;
            const likely_file_type fileType = likely_file_type_from_string(ast->atom, &ok);
            if (ok) {
                const_cast<likely_ast>(ast)->type = likely_ast_type;
                return new likely_expression(LikelyValue(builder.constant(uint64_t(fileType), likely_u32)));
            }
        }

        const_cast<likely_ast>(ast)->type = likely_ast_invalid;
        return likely_expression::error(ast, "invalid atom");
    }
}

likely_env likely_standard(likely_settings settings, likely_mat *output, likely_file_type file_type)
{
    const likely_env env = newEnv(RootEnvironment::get());
    env->settings = (likely_settings*) malloc(sizeof(likely_settings));
    memcpy(env->settings, &settings, sizeof(likely_settings));
    if (output)
        env->module = new OfflineModule(settings, output, file_type);
    return env;
}

likely_env likely_precompiled(likely_const_mat bitcode, const char *symbol)
{
    const likely_env env = newEnv(NULL);
    env->settings = NULL;
    env->module = NULL;
    env->expr = new JITFunction(bitcode, symbol);
    return env;
}

likely_env likely_retain_env(likely_const_env env)
{
    if (!env) return NULL;
    assert(env->ref_count > 0);
    if (env->ref_count != UINT32_MAX)
        const_cast<likely_env>(env)->ref_count++;
    return const_cast<likely_env>(env);
}

void likely_release_env(likely_const_env env)
{
    if (!env) return;
    assert(env->ref_count > 0);
    if ((env->ref_count == UINT32_MAX) || --const_cast<likely_env>(env)->ref_count)
        return;
    likely_expression::release(env->expr);
    likely_release_ast(env->ast);
    if (!env->parent || (env->settings != env->parent->settings))
        free(env->settings);

    // This is where our decision to manage module ownership using the environment plays out.
    // We must check the environment stack to see if we're the last user.
    if (env->module) {
        bool owner = true;
        likely_const_env tmp = env->parent;
        while (tmp) {
            if (env->module == tmp->module) {
                owner = false;
                break;
            }
            tmp = tmp->parent;
        }
        if (owner)
            delete env->module;
    }

    likely_release_env(env->parent);
    free(const_cast<likely_env>(env));
}

likely_mat likely_dynamic(likely_vtable vtable, likely_const_mat *mats, const void *data)
{
    void *function = NULL;
    for (size_t i=0; i<vtable->functions.size(); i++) {
        const unique_ptr<JITFunction> &jitFunction = vtable->functions[i];
        for (size_t j=0; j<vtable->n; j++)
            if (mats[j]->type != jitFunction->parameters[j])
                goto Next;
        function = jitFunction->function;
        if (function == NULL)
            return NULL;
        break;
    Next:
        continue;
    }

    if (function == NULL) {
        vector<likely_type> types = vtable->types;
        int dynamicIndex = 0;
        for (size_t i=0; i<vtable->n; i++)
            if (types[i] == likely_multi_dimension)
                types[i] = mats[dynamicIndex++]->type;
        vtable->functions.push_back(unique_ptr<JITFunction>(new JITFunction("likely_vtable_entry", unique_ptr<Lambda>(new Lambda(vtable->env, vtable->body, vtable->parameters)).get(), types, false, LikelyFunction::VirtualCC, vtable->types)));
        function = vtable->functions.back()->function;
        if (function == NULL)
            return NULL;
    }

    return reinterpret_cast<likely_mat (*)(likely_const_mat const*, const void *)>(function)(mats, data);
}

likely_const_mat likely_result(const struct likely_expression *expr)
{
    if (!expr)
        return NULL;

    if (const likely_const_ast ast = expr->getData())
        return likely_ast_to_string(ast, -1);

    return (likely_const_mat) expr->getData();
}

void *likely_function(const struct likely_expression *expr)
{
    return JITFunction::getFunction(expr);
}

class LazyDefinition : public likely_expression
{
    const likely_const_env env;
    const likely_const_ast ast;

    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        likely_const_env env = this->env;
        swap(builder.env, env);
        UniqueExpression op(get(builder, this->ast));
        swap(builder.env, env);
        if ((ast->type != likely_ast_list) || !op)
            return op.release();
        return op->evaluate(builder, ast);
    }

public:
    LazyDefinition(likely_const_env env, likely_const_ast ast)
        : env(env), ast(ast) {}
};

likely_env likely_eval(likely_ast ast, likely_const_env parent, likely_eval_callback eval_callback, void *context)
{
    if (!ast || (ast->type != likely_ast_list))
        return NULL;

    likely_env env = likely_retain_env(parent);
    for (size_t i=0; i<ast->num_atoms; i++) {
        const likely_ast statement = ast->atoms[i];
        if (!statement)
            continue;

        likely_const_expr expr = NULL;
        env = NULL;
        if (likely_is_definition(statement)) {
            expr = new LazyDefinition(parent, statement->atoms[2]);
        } else {
            // If `ast` is not a lambda then it is a computation we perform by constructing and executing a parameterless lambda.
            const char *const symbol = ((statement->type == likely_ast_list)
                                        && (statement->num_atoms > 0)
                                        && (statement->atoms[0]->type != likely_ast_list))
                                       ? statement->atoms[0]->atom : "";
            if (LikelyFunction::isSymbol(symbol)) {
                Builder builder(parent, parent->module);
                expr = likely_expression::get(builder, statement);
            } else {
                const Variant data = Lambda(parent, statement).evaluateConstantFunction();

                // If the result of a computation is a new environment then use that environment (an import statement for example).
                // Otherwise, construct a new expression from the result.
                if (const likely_const_env evaluated = data) {
                    // Confirm the returned environment is a descendant of `parent`
                    likely_const_env tmp = evaluated->parent;
                    while (tmp && (tmp != parent))
                        tmp = tmp->parent;
                    likely_ensure(tmp != NULL, "evaluation expected a descendant environment.");

                    env = likely_retain_env(evaluated);
                    env->module = parent->module;
                } else {
                    expr = data ? new ConstantData(data) : NULL;
                }
           }
        }

        if (!env)
            env = newEnv(parent, statement, expr);

        likely_release_env(parent);
        parent = env;
        if (eval_callback)
            eval_callback(env, context);
        if (!env->expr)
            break;
    }

    return env;
}

//! [likely_lex_parse_and_eval implementation.]
likely_env likely_lex_parse_and_eval(const char *source, likely_file_type file_type, likely_const_env parent)
{
    const likely_ast ast = likely_lex_and_parse(source, file_type);
    const likely_env env = likely_eval(ast, parent, NULL, NULL);
    likely_release_ast(ast);
    return env;
}
//! [likely_lex_parse_and_eval implementation.]

//! [likely_compute implementation.]
likely_mat likely_compute(const char *source)
{
    const likely_const_env parent = likely_standard(likely_default_settings(likely_file_void, false), NULL, likely_file_void);
    const likely_const_env env = likely_lex_parse_and_eval(source, likely_file_lisp, parent);
    const likely_mat result = likely_retain_mat(likely_result(env->expr));
    likely_release_env(env);
    likely_release_env(parent);
    likely_ensure(result != NULL, "failed to compute: %s", source);
    return result;
}
//! [likely_compute implementation.]

likely_env likely_define(const char *name, likely_const_mat value, likely_const_env parent)
{
    if (!value)
        return NULL;
    likely_expression::define(parent, name, new ConstantData(likely_retain_mat(value)));
    return const_cast<likely_env>(parent); // define() swaps the value of parent with child, so this is safe
}

void likely_shutdown()
{
    likely_release_env(RootEnvironment::get());
    llvm_shutdown();
}
