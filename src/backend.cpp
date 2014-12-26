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

#include <llvm/PassManager.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/AssumptionTracker.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/IRBuilder.h>
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
#include <llvm/Target/TargetLibraryInfo.h>
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

#include "likely/backend.h"

using namespace llvm;
using namespace std;

//! [likely_jit implementation.]
likely_settings likely_jit(bool verbose)
{
    likely_settings settings;
    settings.opt_level = 3;
    settings.size_level = 0;
    settings.heterogeneous = false;
    settings.parallel = likely_can_fork();
    settings.unroll_loops = true;
    settings.vectorize_loops = true;
    settings.verbose = verbose;
    return settings;
}
//! [likely_jit implementation.]

namespace {

class LikelyContext : public likely_settings
{
    map<likely_type, Type*> typeLUT;

public:
    PassManager *PM;
    AssumptionTracker *AT;
    LLVMContext context;

    LikelyContext(const likely_settings &settings)
        : likely_settings(settings), PM(new PassManager()), AT(new AssumptionTracker())
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
        PM->add(AT);
        PM->add(new TargetLibraryInfo(Triple(sys::getProcessTriple())));
        PM->add(new DataLayoutPass());
        TM->addAnalysisPasses(*PM);
        PassManagerBuilder builder;
        builder.OptLevel = opt_level;
        builder.SizeLevel = size_level;
        builder.DisableUnrollLoops = !unroll_loops;
        builder.LoopVectorize = vectorize_loops;
        builder.Inliner = createAlwaysInlinerPass();
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
        if (likely & likely_multi_dimension) {
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
        } else if (likely & likely_ast_t) {
            llvm = PointerType::getUnqual(StructType::create(context, "ast"));
        } else if (likely & likely_env_t) {
            llvm = PointerType::getUnqual(StructType::create(context, "env"));
        } else {
            const uint32_t bits = likely & likely_depth;
            if (likely & likely_floating) {
                if      (bits == 16) llvm = Type::getHalfTy(context);
                else if (bits == 32) llvm = Type::getFloatTy(context);
                else if (bits == 64) llvm = Type::getDoubleTy(context);
                else                 { llvm = NULL; likely_assert(false, "invalid floating bits: %d", bits); }
            } else {
                llvm = Type::getIntNTy(context, bits);
            }
        }

        if (likely & likely_pointer)
            llvm = PointerType::getUnqual(llvm);

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
            likely_assert(TheTarget != NULL, "target lookup failed with error: %s", error.c_str());
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
        likely_assert(TM != NULL, "failed to create target machine");
        return TM;
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
        : value(value), type(type)
    {
        if (value && type) {
            // Check type correctness
            likely_assert(!(type & likely_floating) || !(type & likely_signed), "type can't be both floating and signed (integer)");
            likely_type inferred = toLikely(value->getType());
            if (!(inferred & likely_multi_dimension)) {
                // Can't represent these flags in LLVM IR for scalar types
                if (type & likely_signed)
                    inferred |= likely_signed;
                if (type & likely_saturated)
                    inferred |= likely_saturated;
            }
            if (inferred != type) {
                const likely_mat llvm = likely_type_to_string(inferred);
                const likely_mat likely = likely_type_to_string(type);
                value->dump();
                likely_assert(false, "type mismatch between LLVM: %s and Likely: %s", llvm->data, likely->data);
                likely_release_mat(llvm);
                likely_release_mat(likely);
            }
        }
    }

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
                        likely_type type = toLikely(element);
                        if (!isa<FunctionType>(element))
                            type |= likely_pointer;
                        return type;
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
        : value(value), type(type) {}

    Variant()
        : Variant(NULL, likely_void) {}

    Variant(likely_const_mat mat)
        : Variant((void*) mat, mat ? mat->type : likely_type(likely_void)) {}

    Variant(likely_const_env env)
        : Variant((void*) env, likely_env_t) {}

    ~Variant()
    {
        if      (type & likely_ast_t) likely_release_ast(ast);
        else if (type & likely_env_t) likely_release_env(env);
        else                          likely_release_mat(mat);
    }

    Variant(const Variant &other)
    {
        *this = other;
    }

    Variant &operator=(const Variant &other)
    {
        type = other.type;
        if      (type & likely_ast_t) value = likely_retain_ast(other);
        else if (type & likely_env_t) value = likely_retain_env(other);
        else                          value = likely_retain_mat(other);
        return *this;
    }

    operator bool() const { return value != NULL; }
    operator likely_const_ast() const { return (type & likely_ast_t) ? ast : NULL; }
    operator likely_const_env() const { return (type & likely_env_t) ? env : NULL; }
    operator likely_const_mat() const { return (!(type & (likely_ast_t | likely_env_t))) ? mat : NULL; }
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
    env->settings = parent ? parent->settings : NULL;
    env->module = parent ? parent->module : NULL;
    env->expr = expr;
    env->ref_count = 1;
    return env;
}

struct likely_expression : public LikelyValue
{
    mutable vector<likely_vtable> vtables;

    likely_expression(const LikelyValue &value = LikelyValue(), const Variant &data = Variant())
        : LikelyValue(value), data(data) {}

    virtual ~likely_expression();
    likely_expression(const likely_expression &) = delete;
    likely_expression &operator=(const likely_expression &) = delete;

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

    void setData(const Variant &data) const
    {
        assert(!getData());
        this->data = data;
    }

    virtual likely_const_expr evaluate(Builder &, likely_const_ast) const
    {
        return new likely_expression(LikelyValue(value, type));
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

    static likely_const_expr get(Builder &builder, likely_const_ast ast);

    static likely_const_expr error(likely_const_ast ast, const char *message)
    {
        likely_throw(ast, message);
        return NULL;
    }

    static void define(likely_const_env &env, const char *name, likely_const_expr expr)
    {
        const likely_ast atoms[2] = { likely_atom("=", 1), likely_atom(name, uint32_t(strlen(name))) };
        const likely_ast list = likely_list(atoms, 2);
        env = newEnv(env, list, expr);
        likely_release_ast(list);
    }

    static void undefine(likely_const_env &env, const char *name)
    {
        assert(likely_is_definition(env->ast));
        likely_assert(!strcmp(name, likely_symbol(env->ast)), "undefine variable mismatch");
        const likely_const_env old = env;
        env = env->parent;
        likely_release_env(old);
    }

    static void undefineAll(likely_const_env &env, likely_const_ast args)
    {
        if (args->type == likely_ast_list) {
            for (size_t i=0; i<args->num_atoms; i++)
                undefine(env, args->atoms[args->num_atoms-i-1]->atom);
        } else {
            undefine(env, args->atom);
        }
    }

    static size_t length(likely_const_ast ast)
    {
        return ast ? ((ast->type == likely_ast_list) ? ast->num_atoms : 1) : 0;
    }

private:
    mutable Variant data; // use getData() and setData()
};

struct likely_module
{
    unique_ptr<LikelyContext> context;
    Module *module;
    vector<pair<Variant,Constant*>> data;

    likely_module(const likely_settings &settings)
        : context(new LikelyContext(settings))
        , module(new Module("likely_module", context->context))
    {
        module->setTargetTriple(sys::getProcessTriple());
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
    const string fileName;

public:
    OfflineModule(const likely_settings &settings, const string &fileName)
        : likely_module(settings), fileName(fileName) {}

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
            module->dump();

        error_code errorCode;
        tool_output_file output(fileName.c_str(), errorCode, sys::fs::F_None);
        likely_assert(!errorCode, "%s", errorCode.message().c_str());

        const string extension = fileName.substr(fileName.find_last_of(".") + 1);
        if (extension == "ll") {
            module->print(output.os(), NULL);
        } else if (extension == "bc") {
            WriteBitcodeToFile(module, output.os());
        } else {
            PassManager pm;
            formatted_raw_ostream fos(output.os());
            static TargetMachine *TM = LikelyContext::getTargetMachine(false);
            TM->addPassesToEmitFile(pm, fos, extension == "s" ? TargetMachine::CGFT_AssemblyFile : TargetMachine::CGFT_ObjectFile);
            pm.run(*module);
        }

        output.keep();
    }
};

struct Builder : public IRBuilder<>
{
    likely_const_env env;
    likely_module *const module;
    const bool ctfe; // Compile-time function evaluation

    Builder(likely_const_env env, likely_module *module, bool ctfe)
        : IRBuilder<>(module ? module->context->context : getGlobalContext()), env(env), module(module), ctfe(ctfe) {}

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
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return LikelyValue(); }
        } else {
            return constant(uint64_t(value), type);
        }
    }

    LikelyValue zero(likely_type type = likely_u64) { return constant(0.0, type); }
    LikelyValue one (likely_type type = likely_u64) { return constant(1.0, type); }
    LikelyValue intMax(likely_type type) { const likely_type bits = type & likely_depth; return constant((uint64_t) (1 << (bits - ((type & likely_signed) ? 1 : 0)))-1, bits); }
    LikelyValue intMin(likely_type type) { const likely_type bits = type & likely_depth; return constant((uint64_t) ((type & likely_signed) ? (1 << (bits - 1)) : 0), bits); }
    LikelyValue nullMat() { return LikelyValue(ConstantPointerNull::get(::cast<PointerType>(module->context->toLLVM(likely_multi_dimension))), likely_multi_dimension); }
    LikelyValue nullData() { return LikelyValue(ConstantPointerNull::get(Type::getInt8PtrTy(getContext())), likely_u8 | likely_pointer); }

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

        if (CallInst *const newCall = traceback(m))
            return LikelyValue(newCall->getOperand(1), likely_u32);

        LoadInst *const load = CreateLoad(CreateStructGEP(m, 2), "channels");
        load->setMetadata(LLVMContext::MD_range, axisRange());
        return LikelyValue(load, likely_u32);
    }

    LikelyValue columns(const LikelyValue &m)
    {
        if (!(m & likely_multi_column))
            return one(likely_u32);

        if (CallInst *const newCall = traceback(m))
            return LikelyValue(newCall->getOperand(2), likely_u32);

        LoadInst *const load = CreateLoad(CreateStructGEP(m, 3), "columns");
        load->setMetadata(LLVMContext::MD_range, axisRange());
        return LikelyValue(load, likely_u32);
    }

    LikelyValue rows(const LikelyValue &m)
    {
        if (!(m & likely_multi_row))
            return one(likely_u32);

        if (CallInst *const newCall = traceback(m))
            return LikelyValue(newCall->getOperand(3), likely_u32);

        LoadInst *const load = CreateLoad(CreateStructGEP(m, 4), "rows");
        load->setMetadata(LLVMContext::MD_range, axisRange());
        return LikelyValue(load, likely_u32);
    }

    LikelyValue frames(const LikelyValue &m)
    {
        if (!(m & likely_multi_frame))
            return one(likely_u32);

        if (CallInst *const newCall = traceback(m))
            return LikelyValue(newCall->getOperand(4), likely_u32);

        LoadInst *const load = CreateLoad(CreateStructGEP(m, 5), "frames");
        load->setMetadata(LLVMContext::MD_range, axisRange());
        return LikelyValue(load, likely_u32);
    }

    LikelyValue data(const LikelyValue &m)
    {
        if (!(m & likely_multi_dimension))
            return LikelyValue();

        const likely_type type = (m & likely_element) | likely_pointer;
        return LikelyValue(CreatePointerCast(CreateStructGEP(m, 6), module->context->toLLVM(type)), type);
    }

    LikelyValue cast(const LikelyValue &x, likely_type type)
    {
        type &= likely_element;
        if ((x.type & likely_element) == type)
            return LikelyValue(x, type);
        if ((type & likely_depth) == 0) {
            type |= x.type & likely_depth;
            if (type & likely_floating)
                type = likely_type_from_types(type, likely_floating);
        }
        Type *dstType = module->context->toLLVM(type & likely_element);
        return LikelyValue(CreateCast(CastInst::getCastOpcode(x, (x & likely_signed) != 0, dstType, (type & likely_signed) != 0), x, dstType), type);
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

private:
    static CallInst *traceback(Value *m)
    {
        if (CastInst *const cast = dyn_cast<CastInst>(m))
            m = cast->getOperand(0);
        if (CallInst *const call = dyn_cast<CallInst>(m))
            if (call->getCalledFunction()->getName() == "likely_new")
                return call;
        return NULL;
    }

    MDNode *axisRange()
    {
        Type *const type = Type::getInt32Ty(getContext());
        Value *const values[2] = { ConstantInt::get(type, 1),
                                   ConstantInt::get(type, std::numeric_limits<uint32_t>::max()) };
        return MDNode::get(getContext(), values);
    }
};

class ConstantData : public likely_expression
{
    ConstantData(const Variant &data)
        : likely_expression(LikelyValue(), data) {}

    likely_const_expr evaluate(Builder &builder, likely_const_ast) const
    {
        const Variant data = getData();

        // Special case, return the scalar
        if (const likely_const_mat m = data)
            if (!(m->type & likely_multi_dimension))
                return new likely_expression(LikelyValue(builder.constant(likely_get_element(m, 0, 0, 0, 0), m->type), m->type), data);

        // Make sure the lifetime of the data is at least as long as the lifetime of the code.
        Constant *const address = ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(void*)), uintptr_t(data.value));
        builder.module->data.push_back(pair<Variant,Constant*>(data, address));
        return new likely_expression(LikelyValue(ConstantExpr::getIntToPtr(address, builder.module->context->toLLVM(data.type)), data.type), data);
    }

public:
    static likely_const_expr get(const Variant &data)
    {
        if (!data)
            return NULL;
        return new ConstantData(data);
    }

    static likely_const_expr get(Builder &builder, const Variant &data)
    {
        unique_ptr<const likely_expression> expr(get(data));
        return expr ? expr->evaluate(builder, NULL) : NULL;
    }
};

#define TRY_EXPR(BUILDER, AST, EXPR)                               \
const unique_ptr<const likely_expression> EXPR(get(BUILDER, AST)); \
if (!EXPR.get()) return NULL;                                      \

struct Symbol : public likely_expression
{
    const string name;
    const vector<likely_type> parameters;

    Symbol(const string &name, likely_type returnType, vector<likely_type> parameters = vector<likely_type>())
        : likely_expression(LikelyValue(NULL, returnType)), name(name), parameters(parameters) {}

private:
    size_t maxParameters() const { return parameters.size(); }

    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (parameters.size() != ((ast->type == likely_ast_list) ? ast->num_atoms-1 : 0))
            return error(ast, "incorrect argument count");

        Function *symbol = builder.module->module->getFunction(name);
        if (!symbol) {
            vector<Type*> llvmParameters;
            for (likely_type parameter : parameters)
                llvmParameters.push_back(builder.module->context->toLLVM(parameter));
            Type *llvmReturn = builder.module->context->toLLVM(type);
            FunctionType *functionType = FunctionType::get(llvmReturn, llvmParameters, false);
            symbol = Function::Create(functionType, GlobalValue::ExternalLinkage, name, builder.module->module);
            symbol->setCallingConv(CallingConv::C);
            symbol->setCannotDuplicate();
            symbol->setDoesNotThrow();
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

        vector<Value*> args;
        if (ast->type == likely_ast_list)
            for (size_t i=1; i<ast->num_atoms; i++) {
                TRY_EXPR(builder, ast->atoms[i], arg)
                const likely_type parameter = parameters[i-1];
                if (arg->type & likely_multi_dimension) {
                    if (parameter & likely_multi_dimension)
                        args.push_back(builder.CreatePointerCast(*arg, builder.module->context->toLLVM(parameter)));
                    else if (parameter & likely_pointer)
                        args.push_back(builder.CreatePointerCast(builder.data(*arg), builder.module->context->toLLVM(parameter)));
                    else
                        likely_assert(false, "can't cast matrix to scalar");
                } else {
                    args.push_back(builder.cast(*arg.get(), parameter));
                }
            }

        return new likely_expression(LikelyValue(builder.CreateCall(symbol, args), type));
    }
};

struct Lambda;

struct JITFunction : public Symbol
{
    void *function = NULL;
    ExecutionEngine *EE = NULL;
    likely_module *module;

    JITFunction(const string &name, const Lambda *lambda, const vector<likely_type> &parameters, bool evaluate, bool arrayCC);

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

    struct HasLoop : public LoopInfo
    {
        bool hasLoop = false;

    private:
        bool runOnFunction(Function &F)
        {
            if (hasLoop)
                return false;

            const bool result = LoopInfo::runOnFunction(F);
            hasLoop = !empty();
            return result;
        }
    };
};

class LikelyOperator : public likely_expression
{
    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        if ((ast->type != likely_ast_list) && (minParameters() > 0))
            return error(ast, "operator expected arguments");

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

} // namespace (anonymous)

struct likely_virtual_table
{
    const likely_const_env env;
    const likely_const_ast body, parameters;
    const size_t n;
    vector<unique_ptr<JITFunction>> functions;

    likely_virtual_table(likely_const_env env, likely_const_ast body, likely_const_ast parameters)
        : env(env) /* don't retain a copy because `env` owns us` */, body(likely_retain_ast(body)), parameters(likely_retain_ast(parameters)), n(likely_expression::length(parameters)) {}

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
    virtual likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const = 0;
};

struct MatrixType : public LikelyOperator
{
    likely_type t;
    MatrixType(Builder &builder, likely_type t)
        : t(t)
    {
        value = builder.constant(uint64_t(t), likely_u32);
        type = likely_u32;
    }

private:
    size_t minParameters() const { return 0; }
    size_t maxParameters() const { return 1; }
    static int UID() { return __LINE__; }
    int uid() const { return UID(); }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        if ((ast->type == likely_ast_list) && (ast->num_atoms > 1)) {
            TRY_EXPR(builder, ast->atoms[1], expr)
            if (expr->uid() == UID()) {
                const MatrixType *matrixType = static_cast<const MatrixType*>(expr.get());
                return new MatrixType(builder, likely_type_from_types(matrixType->t, t));
            } else {
                return new likely_expression(builder.cast(*expr, t));
            }
        } else {
            return new likely_expression((LikelyValue) *this);
        }
    }
};

#define LIKELY_REGISTER_AXIS(AXIS)                                                   \
class AXIS##Expression : public LikelyOperator                                       \
{                                                                                    \
    const char *symbol() const { return #AXIS; }                                     \
    size_t maxParameters() const { return 1; }                                       \
    size_t minParameters() const { return 0; }                                       \
                                                                                     \
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const \
    {                                                                                \
        if (length(ast) < 2) {                                                       \
            return new AXIS##Expression();                                           \
        } else {                                                                     \
            TRY_EXPR(builder, ast->atoms[1], expr)                                   \
            return new likely_expression(builder.AXIS(*expr.get()));                 \
        }                                                                            \
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
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(LikelyValue(builder.CreateXor(builder.intMax(*arg), arg->value), *arg));
    }
};
LIKELY_REGISTER(not)

class typeExpression : public SimpleUnaryOperator
{
    const char *symbol() const { return "type"; }
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new MatrixType(builder, *arg);
    }
};
LIKELY_REGISTER(type)

class UnaryMathOperator : public SimpleUnaryOperator
{
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &x) const
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
    virtual likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &arg1, const unique_ptr<const likely_expression> &arg2) const = 0;
};

class ArithmeticOperator : public SimpleBinaryOperator
{
    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &lhs, const unique_ptr<const likely_expression> &rhs) const
    {
        likely_type type = likely_type_from_types(*lhs, *rhs);
        return evaluateArithmetic(builder, builder.cast(*lhs.get(), type), builder.cast(*rhs.get(), type));
    }
    virtual likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const = 0;
};

class SimpleArithmeticOperator : public ArithmeticOperator
{
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        return new likely_expression(LikelyValue(evaluateSimpleArithmetic(builder, lhs, rhs), lhs));
    }
    virtual Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const = 0;
};

class addExpression : public SimpleArithmeticOperator
{
    const char *symbol() const { return "+"; }
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
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
        unique_ptr<const likely_expression> expr1, expr2;
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
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
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
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &n, const likely_expression &d) const
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
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        return (lhs.type & likely_floating) ? builder.CreateFRem(lhs, rhs)
                                                   : ((lhs.type & likely_signed) ? builder.CreateSRem(lhs, rhs)
                                                                                 : builder.CreateURem(lhs, rhs));
    }
};
LIKELY_REGISTER(remainder)

#define LIKELY_REGISTER_LOGIC(OP, SYM)                                                                                  \
class OP##Expression : public SimpleArithmeticOperator                                                                  \
{                                                                                                                       \
    const char *symbol() const { return #SYM; }                                                                         \
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const \
    {                                                                                                                   \
        return builder.Create##OP(lhs, rhs.value);                                                                      \
    }                                                                                                                   \
};                                                                                                                      \
LIKELY_REGISTER(OP)                                                                                                     \

LIKELY_REGISTER_LOGIC(And , &)
LIKELY_REGISTER_LOGIC(Or  , |)
LIKELY_REGISTER_LOGIC(Xor , ^)
LIKELY_REGISTER_LOGIC(Shl , <<)

class shiftRightExpression : public SimpleArithmeticOperator
{
    const char *symbol() const { return ">>"; }
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        return (lhs.type & likely_signed) ? builder.CreateAShr(lhs, rhs.value) : builder.CreateLShr(lhs, rhs.value);
    }
};
LIKELY_REGISTER(shiftRight)

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                                                    \
class OP##Expression : public ArithmeticOperator                                                                                                               \
{                                                                                                                                                              \
    const char *symbol() const { return #SYM; }                                                                                                                \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const                                   \
    {                                                                                                                                                          \
        return new likely_expression(LikelyValue((lhs.type & likely_floating) ? builder.CreateFCmpO##OP(lhs, rhs)                                              \
                                                                              : ((lhs.type & likely_signed) ? builder.CreateICmpS##OP(lhs, rhs)                \
                                                                                                            : builder.CreateICmpU##OP(lhs, rhs)), likely_u1)); \
    }                                                                                                                                                          \
};                                                                                                                                                             \
LIKELY_REGISTER(OP)                                                                                                                                            \

LIKELY_REGISTER_COMPARISON(LT, <)
LIKELY_REGISTER_COMPARISON(LE, <=)
LIKELY_REGISTER_COMPARISON(GT, >)
LIKELY_REGISTER_COMPARISON(GE, >=)

#define LIKELY_REGISTER_EQUALITY(OP, SYM)                                                                                      \
class OP##Expression : public ArithmeticOperator                                                                               \
{                                                                                                                              \
    const char *symbol() const { return #SYM; }                                                                                \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const   \
    {                                                                                                                          \
        return new likely_expression(LikelyValue((lhs.type & likely_floating) ? builder.CreateFCmpO##OP(lhs, rhs)              \
                                                                              : builder.CreateICmp##OP(lhs, rhs), likely_u1)); \
    }                                                                                                                          \
};                                                                                                                             \
LIKELY_REGISTER(OP)                                                                                                            \

LIKELY_REGISTER_EQUALITY(EQ, ==)
LIKELY_REGISTER_EQUALITY(NE, !=)

class BinaryMathOperator : public SimpleBinaryOperator
{
    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &x, const unique_ptr<const likely_expression> &n) const
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
        likely_const_expr value = NULL;
        likely_retain_ast(ast->atoms[1]);
        const likely_ast statement = likely_list(&ast->atoms[1], 1);
        if (const likely_env env = likely_eval(statement, builder.env, NULL, NULL)) {
            if (const likely_const_mat mat = likely_result(env->expr))
                value = ConstantData::get(builder, likely_retain_mat(mat));
            likely_release_env(env);
        }
        likely_release_ast(statement);

        if (!value)
            value = get(builder, ast->atoms[2]);
        return value;
    }
};
LIKELY_REGISTER(try)

struct Lambda : public LikelyOperator
{
    const likely_const_env env;
    const likely_const_ast body, parameters;

    Lambda(likely_const_env env, likely_const_ast body, likely_const_ast parameters = NULL)
        : env(likely_retain_env(env)), body(likely_retain_ast(body)), parameters(likely_retain_ast(parameters)) {}

    ~Lambda()
    {
        likely_release_ast(parameters);
        likely_release_ast(body);
        likely_release_env(env);
    }

    static bool isFunction(const char *symbol)
    {
        return !strcmp(symbol, "->") || !strcmp(symbol, "extern");
    }

    likely_const_expr generate(Builder &builder, vector<likely_type> parameters, string name, bool arrayCC, bool promoteScalarToMatrix) const
    {
        while (parameters.size() < maxParameters())
            parameters.push_back(likely_multi_dimension);

        vector<Type*> llvmTypes;
        if (arrayCC) {
            // Array calling convention - All arguments (which must be matrix pointers) come stored in an array.
            llvmTypes.push_back(PointerType::get(builder.module->context->toLLVM(likely_multi_dimension), 0));
        } else {
            for (const likely_type &parameter : parameters)
                llvmTypes.push_back(builder.module->context->toLLVM(parameter));
        }

        BasicBlock *originalInsertBlock = builder.GetInsertBlock();
        Function *tmpFunction = cast<Function>(builder.module->module->getOrInsertFunction(name+"_tmp", FunctionType::get(Type::getVoidTy(builder.getContext()), llvmTypes, false)));
        BasicBlock *entry = BasicBlock::Create(builder.getContext(), "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        vector<likely_const_expr> arguments;
        if (arrayCC) {
            Value *argumentArray = tmpFunction->arg_begin();
            for (size_t i=0; i<parameters.size(); i++) {
                Value *load = builder.CreateLoad(builder.CreateGEP(argumentArray, builder.constant(i)));
                if (parameters[i] & likely_multi_dimension) {
                    load = builder.CreatePointerCast(load, builder.module->context->toLLVM(parameters[i]));
                } else {
                    const likely_type tmpType = parameters[i] | likely_multi_dimension;
                    const LikelyValue tmpValue(builder.CreatePointerCast(load, builder.module->context->toLLVM(tmpType)), tmpType);
                    load = builder.CreateLoad(builder.CreateGEP(builder.data(tmpValue), builder.zero()));
                }
                arguments.push_back(new likely_expression(LikelyValue(load, parameters[i])));
            }
        } else {
            Function::arg_iterator it = tmpFunction->arg_begin();
            size_t i = 0;
            while (it != tmpFunction->arg_end())
                arguments.push_back(new likely_expression(LikelyValue(it++, parameters[i++])));
        }

        for (size_t i=0; i<arguments.size(); i++) {
            stringstream name; name << "arg_" << i;
            arguments[i]->value->setName(name.str());
        }

        unique_ptr<const likely_expression> result(evaluateFunction(builder, arguments));
        if (!result)
            return NULL;

        // If we are expecting a constant or a matrix and don't get one then make a matrix
        if (promoteScalarToMatrix && !result->getData() && !dyn_cast<PointerType>(result->value->getType()))
            result.reset(new likely_expression(builder.toMat(*result)));

        // If we are returning a constant matrix, make sure to retain a copy
        if (isa<ConstantExpr>(result->value) && isMat(result->value->getType()))
            result.reset(new likely_expression(LikelyValue(builder.CreatePointerCast(builder.retainMat(result->value), builder.module->context->toLLVM(result->type)), result->type), result->getData()));

        builder.CreateRet(*result);

        Function *function = cast<Function>(builder.module->module->getOrInsertFunction(name, FunctionType::get(result->value->getType(), llvmTypes, false)));

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

    Variant evaluateConstantFunction(const vector<likely_const_mat> &args = vector<likely_const_mat>()) const
    {
        vector<likely_type> params;
        for (likely_const_mat arg : args)
            params.push_back(arg->type);

        JITFunction jit("likely_ctfe", this, params, true, !args.empty());
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

private:
    size_t maxParameters() const { return length(parameters); }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_expr result = NULL;

        vector<likely_const_expr> args;
        vector<likely_const_mat> constantArgs;
        const size_t arguments = length(ast)-1;
        for (size_t i=0; i<arguments; i++) {
            likely_const_expr arg = get(builder, ast->atoms[i+1]);
            if (!arg)
                goto cleanup;

            if (builder.ctfe && (constantArgs.size() == args.size()))
                if (likely_const_mat constantArg = arg->getData())
                    constantArgs.push_back(constantArg);

            args.push_back(arg);
        }

        result = (builder.ctfe && !isFunction(likely_symbol(body)) && (constantArgs.size() == args.size()))
                 ? ConstantData::get(builder, evaluateConstantFunction(constantArgs))
                 : evaluateFunction(builder, args);

    cleanup:
        for (likely_const_expr arg : args)
            delete arg;
        return result;
    }

    likely_const_expr evaluateFunction(Builder &builder, vector<likely_const_expr> &args /* takes ownership */) const
    {
        assert(args.size() == maxParameters());

        // Do dynamic dispatch if the type isn't fully specified
        bool dynamic = false;
        for (likely_const_expr arg : args)
            dynamic |= (arg->type == likely_multi_dimension);

        if (dynamic) {
            const likely_vtable vtable = new likely_virtual_table(env, body, parameters);
            if (!env->expr)
                const_cast<likely_expr&>(env->expr) = new likely_expression();
            env->expr->vtables.push_back(vtable);

            PointerType *vTableType = PointerType::getUnqual(StructType::create(builder.getContext(), "VTable"));
            Function *likelyDynamic = builder.module->module->getFunction("likely_dynamic");
            if (!likelyDynamic) {
                Type *params[] = { vTableType, PointerType::get(builder.module->context->toLLVM(likely_multi_dimension), 0) };
                FunctionType *likelyDynamicType = FunctionType::get(builder.module->context->toLLVM(likely_multi_dimension), params, false);
                likelyDynamic = Function::Create(likelyDynamicType, GlobalValue::ExternalLinkage, "likely_dynamic", builder.module->module);
                likelyDynamic->setCallingConv(CallingConv::C);
                likelyDynamic->setDoesNotAlias(0);
                likelyDynamic->setDoesNotAlias(1);
                likelyDynamic->setDoesNotCapture(1);
                likelyDynamic->setDoesNotAlias(2);
                likelyDynamic->setDoesNotCapture(2);
                sys::DynamicLibrary::AddSymbol("likely_dynamic", (void*) likely_dynamic);
            }

            Value *matricies = builder.CreateAlloca(builder.module->context->toLLVM(likely_multi_dimension), builder.constant(args.size()));
            for (size_t i=0; i<args.size(); i++)
                builder.CreateStore(*args[i], builder.CreateGEP(matricies, builder.constant(i)));
            Value* args[] = { ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(vtable)), uintptr_t(vtable)), vTableType), matricies };
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

        if (ast->num_atoms < 5)
            return new Symbol(name, returnType, parameters);

        const bool arrayCC = (ast->num_atoms >= 6) && (evalInt(ast->atoms[5], builder.env, &ok) != 0);
        if (!ok)
            return NULL;

        const Lambda *lambda = static_cast<const Lambda*>((ast->atoms[4]->type == likely_ast_list) ? likely_expression::get(builder, ast->atoms[4])
                                                                                                   : lookup(builder.env, ast->atoms[4]->atom)->expr);
        const unique_ptr<const Lambda> lambdaOwner((ast->atoms[4]->type == likely_ast_list) ? lambda : NULL);
        if (builder.module /* static compilation */) {
            if (const likely_const_expr function = lambda->generate(builder, parameters, name, arrayCC, false)) {
                const likely_const_expr symbol = new Symbol(name, function->type, parameters);
                delete function;
                return symbol;
            }
        } else /* JIT compilation */ {
            JITFunction *jitFunction = new JITFunction(name, lambda, parameters, false, arrayCC);
            if (jitFunction->function) {
                sys::DynamicLibrary::AddSymbol(name, jitFunction->function);
                return jitFunction;
            } else {
                delete jitFunction;
            }
        }
        return NULL;
    }
};
LIKELY_REGISTER(extern)

class beginExpression : public LikelyOperator
{
    const char *symbol() const { return "{"; }
    size_t minParameters() const { return 1; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_expr result = NULL;
        likely_const_env root = builder.env;
        for (size_t i=1; i<ast->num_atoms-1; i++) {
            const unique_ptr<const likely_expression> expr(get(builder, ast->atoms[i]));
            if (!expr.get())
                goto cleanup;
        }
        result = get(builder, ast->atoms[ast->num_atoms-1]);

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

class labelExpression : public LikelyOperator
{
    const char *symbol() const { return "#"; }
    size_t maxParameters() const { return 1; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const string name = ast->atoms[1]->atom;
        BasicBlock *label = BasicBlock::Create(builder.getContext(), name, builder.GetInsertBlock()->getParent());
        builder.CreateBr(label);
        builder.SetInsertPoint(label);
        define(builder.env, name.c_str(), new Label(label));
        return new Label(label);
    }
};
LIKELY_REGISTER(label)

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
        BasicBlock *True = BasicBlock::Create(builder.getContext(), "then", function);
        BasicBlock *False = hasElse ? BasicBlock::Create(builder.getContext(), "else", function) : NULL;
        BasicBlock *End = BasicBlock::Create(builder.getContext(), "end", function);
        builder.CreateCondBr(*Cond, True, hasElse ? False : End);

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
            PHINode *phi = builder.CreatePHI(builder.module->context->toLLVM(resolved), 2);
            phi->addIncoming(tc, True);
            phi->addIncoming(fc, False);
            return new likely_expression(LikelyValue(phi, resolved));
        } else {
            if (True->empty() || !True->back().isTerminator())
                builder.CreateBr(End);
            builder.SetInsertPoint(End);
            return new likely_expression();
        }
    }
};
LIKELY_REGISTER(if)

struct Loop : public likely_expression
{
    string name;
    Value *start, *stop, *precondition, *postcondition;
    BasicBlock *entry, *body, *exit;
    BranchInst *latch;

    Loop(Builder &builder, const string &name, Value *start, Value *stop)
        : name(name), start(start), stop(stop), exit(NULL), latch(NULL)
    {
        entry = builder.GetInsertBlock();
        body = BasicBlock::Create(builder.getContext(), name + "_body", entry->getParent());
        exit = BasicBlock::Create(builder.getContext(), name + "_exit", body->getParent());
        precondition = builder.CreateICmpNE(start, stop, name + "_precondition");
        builder.CreateCondBr(precondition, body, exit);
        builder.SetInsertPoint(body);
        type = toLikely(start->getType());
        value = builder.CreatePHI(builder.module->context->toLLVM(type), 2, name);
        cast<PHINode>(value)->addIncoming(start, entry);
    }

    virtual void close(Builder &builder)
    {
        Value *const increment = builder.addInts(value, builder.one(type), name + "_increment");
        postcondition = builder.CreateICmpNE(increment, stop, name + "_postcondition");
        latch = builder.CreateCondBr(postcondition, body, exit);
        cast<PHINode>(value)->addIncoming(increment, builder.GetInsertBlock());
        builder.SetInsertPoint(exit);
    }
};

class loopExpression : public LikelyOperator
{
    const char *symbol() const { return "$"; }
    size_t maxParameters() const { return 3; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[3], end)
        Loop *const loop = new Loop(builder, ast->atoms[2]->atom, builder.zero(*end), *end);
        define(builder.env, ast->atoms[2]->atom, loop); // takes ownership of `loop`
        likely_const_expr expression = get(builder, ast->atoms[1]);
        loop->close(builder);
        undefine(builder.env, ast->atoms[2]->atom);
        return expression;
    }
};
LIKELY_REGISTER(loop)

struct Assignable : public LikelyOperator
{
    Assignable(Value *value, likely_type type)
    {
        this->value = value;
        this->type = type;
    }

    virtual void set(Builder &builder, likely_const_expr expr, likely_const_ast ast) const = 0;

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

class kernelExpression : public LikelyOperator
{
    struct KernelInfo
    {
        likely_type type;
        MDNode *node = NULL;
        Value *c = NULL, *x = NULL, *y = NULL, *t = NULL;
        Value *columnStep = NULL, *rowStep = NULL, *frameStep = NULL;
        Value *cOffset = NULL, *xOffset = NULL, *yOffset = NULL, *tOffset = NULL;
    };

    struct KernelArgument : public Assignable
    {
        const string name;
        KernelInfo info;
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
                Function *const assume = Intrinsic::getDeclaration(builder.module->module, Intrinsic::assume);
                CallInst *const assumption = builder.CreateCall(assume, maskcond);
                builder.module->context->AT->registerAssumption(assumption);
            }
        }

    private:
        Value *gep(Builder &builder, likely_const_ast ast) const
        {
            const size_t len = length(ast);
            const likely_type sharedAxes = ~(type ^ info.type) & likely_multi_dimension;
            const size_t sharedOffset = (sharedAxes == likely_multi_dimension) ? 5 - len : 0;

            // Use the kernel offset for axes that aren't specified
            Value *i;
            if      (sharedOffset == 4) i = info.cOffset;
            else if (sharedOffset == 3) i = info.xOffset;
            else if (sharedOffset == 2) i = info.yOffset;
            else if (sharedOffset == 1) i = info.tOffset;
            else                        i = builder.zero();

            // Compute our own offset for axes that are specified
            if ((sharedOffset < 1) && (type & likely_multi_frame)) {
                Value *const t = (len >= 5) ? builder.cast(*unique_ptr<const likely_expression>(get(builder, ast->atoms[4])), likely_u64).value
                                            : info.t;
                Value *const tStep = ((sharedAxes & likely_multi_channel)
                                      && (sharedAxes & likely_multi_column)
                                      && (sharedAxes & likely_multi_row)
                                      && (sharedAxes & likely_multi_frame)) ? info.frameStep : frameStep;
                i = builder.addInts(builder.multiplyInts(t, tStep), i);
            }
            if ((sharedOffset < 2) && (type & likely_multi_row)) {
                Value *const y = (len >= 4) ? builder.cast(*unique_ptr<const likely_expression>(get(builder, ast->atoms[3])), likely_u64).value
                                            : info.y;
                Value *const yStep = ((sharedAxes & likely_multi_channel)
                                      && (sharedAxes & likely_multi_column)
                                      && (sharedAxes & likely_multi_row)) ? info.rowStep : rowStep;
                i = builder.addInts(builder.multiplyInts(y, yStep), i);
            }
            if ((sharedOffset < 3) && (type & likely_multi_column)) {
                Value *const x = (len >= 3) ? builder.cast(*unique_ptr<const likely_expression>(get(builder, ast->atoms[2])), likely_u64).value
                                            : info.x;
                Value *const xStep = ((sharedAxes & likely_multi_channel)
                                      && (sharedAxes & likely_multi_column)) ? info.columnStep : channels;
                i = builder.addInts(builder.multiplyInts(x, xStep), i);
            }
            if ((sharedOffset < 4) && (type & likely_multi_channel)) {
                Value *const c = (len >= 2) ? builder.cast(*unique_ptr<const likely_expression>(get(builder, ast->atoms[1])), likely_u64).value
                                            : info.c;
                i = builder.addInts(c, i);
            }

            return builder.CreateGEP(data, i);
        }

        void set(Builder &builder, likely_const_expr expr, likely_const_ast ast) const
        {
            StoreInst *store = builder.CreateStore(builder.cast(*expr, type), gep(builder, ast));
            store->setMetadata("llvm.mem.parallel_loop_access", info.node);
        }

        likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
        {
            if (!isa<PointerType>(value->getType()))
                return new likely_expression((LikelyValue) *this);

            LoadInst *load = builder.CreateLoad(gep(builder, ast));
            load->setMetadata("llvm.mem.parallel_loop_access", info.node);
            return new likely_expression(LikelyValue(load, type & likely_element));
        }
    };

    struct KernelAxis : public Loop
    {
        KernelAxis *parent, *child;
        MDNode *node;
        Value *step, *offset;

        KernelAxis(Builder &builder, const string &name, Value *start, Value *stop, Value *step, KernelAxis *parent)
            : Loop(builder, name, start, stop), parent(parent), child(NULL), step(step)
        {
            { // Create self-referencing loop node
                vector<Value*> metadata;
                MDNode *tmp = MDNode::getTemporary(builder.getContext(), metadata);
                metadata.push_back(tmp);
                node = MDNode::get(builder.getContext(), metadata);
                tmp->replaceAllUsesWith(node);
                MDNode::deleteTemporary(tmp);
            }

            offset = builder.addInts(parent ? parent->offset : builder.zero().value, builder.multiplyInts(step, value));
            offset->setName(name + "_offset");

            if (parent)
                parent->child = this;
        }

        void close(Builder &builder)
        {
            Loop::close(builder);
            latch->setMetadata("llvm.loop", node);
            if (child) exit->moveAfter(child->exit);
            if (parent) parent->close(builder);
        }

        void tryCollapse(Builder &builder)
        {
            if ((value->getNumUses() == 2) && child && (child->value->getNumUses() == 2)) {
                // Assume for now that one user is the offset and the other is the increment.

                // Update our range
                BasicBlock *const restore = builder.GetInsertBlock();
                builder.SetInsertPoint(cast<Instruction>(precondition));
                Value *const newStart = builder.multiplyInts(start, child->stop);
                Value *const newStop = builder.multiplyInts(stop, child->stop);
                builder.SetInsertPoint(cast<Instruction>(offset));
                Value *const newOffset = builder.addInts(parent ? parent->offset : builder.zero().value, builder.multiplyInts(child->step, value));
                cast<ICmpInst>(precondition)->setOperand(0, newStart);
                cast<ICmpInst>(precondition)->setOperand(1, newStop);
                cast<PHINode>(value)->setIncomingValue(0, newStart);
                cast<ICmpInst>(postcondition)->setOperand(1, newStop);
                offset->replaceAllUsesWith(newOffset);
                start = newStart;
                stop = newStop;
                step = child->step;
                offset = newOffset;
                builder.SetInsertPoint(restore);

                // Collapse the child loop
                child->value->replaceAllUsesWith(builder.zero());
                child->latch->setCondition(ConstantInt::getFalse(builder.getContext()));
                child->precondition->replaceAllUsesWith(ConstantInt::getTrue(builder.getContext()));
            }

            if (parent)
                parent->tryCollapse(builder);
        }
    };

    const char *symbol() const { return "=>"; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        vector<likely_const_expr> srcs;
        const likely_const_ast args = ast->atoms[1];
        if (args->type == likely_ast_list) {
            for (size_t j=0; j<args->num_atoms; j++)
                srcs.push_back(get(builder, args->atoms[j]));
        } else {
            srcs.push_back(get(builder, args));
        }

        Value *kernelSize;
        if      (srcs[0]->type & likely_multi_frame)   kernelSize = builder.cast(builder.frames  (*srcs[0]), likely_u64);
        else if (srcs[0]->type & likely_multi_row)     kernelSize = builder.cast(builder.rows    (*srcs[0]), likely_u64);
        else if (srcs[0]->type & likely_multi_column)  kernelSize = builder.cast(builder.columns (*srcs[0]), likely_u64);
        else if (srcs[0]->type & likely_multi_channel) kernelSize = builder.cast(builder.channels(*srcs[0]), likely_u64);
        else                                           kernelSize = builder.one();

        if      (builder.module->context->heterogeneous) generateHeterogeneous(builder, ast, srcs, kernelSize);
        else if (builder.module->context->parallel)      generateParallel     (builder, ast, srcs, kernelSize);
        else                                             generateSerial       (builder, ast, srcs, kernelSize);

        for (size_t i=1; i<srcs.size(); i++)
            delete srcs[i];
        return srcs[0];
    }

    void generateSerial(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, Value *kernelSize) const
    {
        generateCommon(builder, ast, srcs, builder.zero(), kernelSize);
    }

    void generateParallel(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, Value *kernelSize) const
    {
        BasicBlock *entry = builder.GetInsertBlock();

        vector<Type*> parameterTypes;
        for (const likely_const_expr src : srcs)
            parameterTypes.push_back(src->value->getType());
        StructType *parameterStructType = StructType::get(builder.getContext(), parameterTypes);

        Function *thunk;
        {
            Type *params[] = { PointerType::getUnqual(parameterStructType), builder.module->context->toLLVM(likely_u64), builder.module->context->toLLVM(likely_u64) };
            FunctionType *thunkType = FunctionType::get(Type::getVoidTy(builder.getContext()), params, false);

            thunk = ::cast<Function>(builder.module->module->getOrInsertFunction(builder.GetInsertBlock()->getParent()->getName().str() + "_thunk", thunkType));
            thunk->addFnAttr(Attribute::NoUnwind);
            thunk->setCallingConv(CallingConv::C);
            thunk->setLinkage(GlobalValue::PrivateLinkage);
            thunk->setDoesNotAlias(1);
            thunk->setDoesNotCapture(1);

            Function::arg_iterator it = thunk->arg_begin();
            Value *parameterStruct = it++;
            Value *start = it++;
            Value *stop = it++;

            builder.SetInsertPoint(BasicBlock::Create(builder.getContext(), "entry", thunk));
            vector<likely_const_expr> thunkSrcs;
            for (size_t i=0; i<srcs.size(); i++)
                thunkSrcs.push_back(new likely_expression(LikelyValue(builder.CreateLoad(builder.CreateStructGEP(parameterStruct, unsigned(i))), srcs[i]->type)));

            generateCommon(builder, ast, thunkSrcs, start, stop);
            const_cast<likely_expr>(srcs[0])->type = thunkSrcs[0]->type;
            for (likely_const_expr thunkSrc : thunkSrcs)
                delete thunkSrc;
            builder.CreateRetVoid();
        }

        builder.SetInsertPoint(entry);

        Type *params[] = { thunk->getType(), PointerType::getUnqual(parameterStructType), builder.module->context->toLLVM(likely_u64) };
        FunctionType *likelyForkType = FunctionType::get(Type::getVoidTy(builder.getContext()), params, false);
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

        Value *parameterStruct = builder.CreateAlloca(parameterStructType);
        for (size_t i=0; i<srcs.size(); i++)
            builder.CreateStore(*srcs[i], builder.CreateStructGEP(parameterStruct, unsigned(i)));

        builder.CreateCall3(likelyFork, builder.module->module->getFunction(thunk->getName()), parameterStruct, kernelSize);
    }

    void generateHeterogeneous(Builder &, likely_const_ast, const vector<likely_const_expr> &, Value *) const
    {
        assert(!"Not implemented");
    }

    void generateCommon(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, Value *start, Value *stop) const
    {
        BasicBlock *kernelHead = BasicBlock::Create(builder.getContext(), "kernel_head", builder.GetInsertBlock()->getParent());
        builder.CreateBr(kernelHead);
        builder.SetInsertPoint(kernelHead);

        vector<KernelArgument*> kernelArguments;
        const likely_const_ast args = ast->atoms[1];
        if (args->type == likely_ast_list) {
            for (size_t i=0; i<args->num_atoms; i++)
                kernelArguments.push_back(new KernelArgument(builder, *srcs[i], args->atoms[i]->atom));
        } else {
            kernelArguments.push_back(new KernelArgument(builder, *srcs[0], args->atom));
        }

        KernelInfo info;
        KernelAxis *axis = NULL;
        if (kernelArguments[0]->type & likely_multi_frame) {
            axis = new KernelAxis(builder, "t", start, stop, kernelArguments[0]->frameStep, NULL);
            info.t = axis->value;
            define(builder.env, "t", axis);
        } else {
            info.t = builder.zero();
            define(builder.env, "t", new likely_expression(builder.zero()));
        }
        info.tOffset = axis ? axis->offset : builder.one().value;

        if (kernelArguments[0]->type & likely_multi_row) {
            axis = new KernelAxis(builder, "y", axis ? builder.zero().value : start, axis ? kernelArguments[0]->rows : stop, kernelArguments[0]->rowStep, axis);
            info.y = axis->value;
            define(builder.env, "y", axis);
        } else {
            info.y = builder.zero();
            define(builder.env, "y", new likely_expression(builder.zero()));
        }
        info.yOffset = axis ? axis->offset : builder.one().value;

        if (kernelArguments[0]->type & likely_multi_column) {
            axis = new KernelAxis(builder, "x", axis ? builder.zero().value : start, axis ? kernelArguments[0]->columns : stop, kernelArguments[0]->channels, axis);
            info.x = axis->value;
            define(builder.env, "x", axis);
        } else {
            info.x = builder.zero();
            define(builder.env, "x", new likely_expression(builder.zero()));
        }
        info.xOffset = axis ? axis->offset : builder.one().value;

        if ((kernelArguments[0]->type & likely_multi_channel) || !axis) {
            axis = new KernelAxis(builder, "c", axis ? builder.zero().value : start, axis ? kernelArguments[0]->channels : stop, builder.one(), axis);
            info.c = axis->value;
            define(builder.env, "c", axis);
        } else {
            info.c = builder.zero();
            define(builder.env, "c", new likely_expression(builder.zero()));
        }
        info.cOffset = axis->offset;

        info.type = kernelArguments[0]->type;
        info.columnStep = kernelArguments[0]->channels;
        info.rowStep = kernelArguments[0]->rowStep;
        info.frameStep = kernelArguments[0]->frameStep;
        info.node = axis->node;
        for (KernelArgument *kernelArgument : kernelArguments) {
            kernelArgument->info = info;
            define(builder.env, kernelArgument->name.c_str(), kernelArgument);
        }

        delete get(builder, ast->atoms[2]);

        undefineAll(builder.env, args);
        kernelArguments.clear();

        axis->close(builder);

        // Clean up any instructions we didn't end up using
        FunctionPassManager FPM(builder.module->module);
        FPM.add(createDeadInstEliminationPass());
        FPM.run(*kernelHead->getParent());

        axis->tryCollapse(builder);
        undefine(builder.env, "c");
        undefine(builder.env, "x");
        undefine(builder.env, "y");
        undefine(builder.env, "t");
    }
};
LIKELY_REGISTER(kernel)

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
        return new likely_expression((LikelyValue) *expr);
    }
};
LIKELY_REGISTER(define)

class setExpression : public LikelyOperator
{
    struct Variable : public Assignable
    {
        Variable(Builder &builder, likely_const_expr expr, const string &name)
            : Assignable(builder.CreateAlloca(builder.module->context->toLLVM(expr->type), 0, name), *expr)
        {
            set(builder, expr);
        }

    private:
        void set(Builder &builder, likely_const_expr expr, likely_const_ast = NULL) const
        {
            builder.CreateStore((type & likely_multi_dimension) ? expr->value
                                                                : builder.cast(*expr, type).value, value);
        }

        likely_const_expr evaluateOperator(Builder &builder, likely_const_ast) const
        {
            return new likely_expression(LikelyValue(builder.CreateLoad(value), type));
        }
    };

    const char *symbol() const { return "<-"; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast lhs = ast->atoms[1];
        likely_const_ast rhs = ast->atoms[2];
        const char *name = likely_symbol(ast);
        assert(builder.module);
        likely_const_expr expr = get(builder, rhs);
        if (expr) {
            const likely_const_env env = lookup(builder.env, name);
            const Assignable *assignable = env ? Assignable::dynamicCast(env->expr) : NULL;
            if (assignable) assignable->set(builder, expr, lhs);
            else            define(builder.env, name, new Variable(builder, expr, name));
        }
        return expr;
    }
};
LIKELY_REGISTER(set)

JITFunction::JITFunction(const string &name, const Lambda *lambda, const vector<likely_type> &parameters, bool evaluate, bool arrayCC)
    : Symbol(name, likely_void, parameters)
    , module(new likely_module((evaluate || !lambda->env->settings) // standard library does not have settings
                               ? likely_jit(lambda->env->settings && lambda->env->settings->verbose)
                               : *lambda->env->settings))
{
    Builder builder(lambda->env, module, !evaluate);
    {
        unique_ptr<const likely_expression> expr(lambda->generate(builder, parameters, name, arrayCC, evaluate));
        if (expr) {
            value = expr->value;
            type = expr->type;
            setData(expr->getData());
        }
    }
    if (!value /* error */ || (evaluate && getData()) /* constant */)
        return;

// No libffi support for Windows
#ifdef _WIN32
    evaluate = false;
#endif // _WIN32

    // Don't run the interpreter on a module with loops, better to compile and execute it instead.
    if (evaluate) {
        PassManager PM;
        HasLoop *hasLoop = new HasLoop();
        PM.add(hasLoop);
        PM.run(*builder.module->module);
        evaluate = !hasLoop->hasLoop;
    }

    TargetMachine *targetMachine = LikelyContext::getTargetMachine(true);
    builder.module->module->setDataLayout(targetMachine->getSubtargetImpl()->getDataLayout());

    string error;
    EngineBuilder engineBuilder(unique_ptr<Module>(builder.module->module));
    engineBuilder.setErrorStr(&error)
                 .setEngineKind(evaluate ? EngineKind::Interpreter : EngineKind::JIT);

    EE = engineBuilder.create(targetMachine);
    likely_assert(EE != NULL, "failed to create execution engine with error: %s", error.c_str());

    if (!evaluate) {
        // optimize
        EE->setObjectCache(&TheJITFunctionCache);
        if (!TheJITFunctionCache.alert(builder.module->module))
            builder.module->context->PM->run(*builder.module->module);
    }

    if (module->context->verbose)
        builder.module->module->dump();

    if (!evaluate) {
        // compile & cleanup
        EE->finalizeObject();
        function = (void*) EE->getFunctionAddress(name);
        EE->removeModule(builder.module->module);
        builder.module->finalize();
    }
}

} // namespace (anonymous)

likely_expression::~likely_expression()
{
    for (likely_vtable vtable : vtables)
        delete vtable;
}

// As a special exception, this function is allowed to set ast->type
likely_const_expr likely_expression::get(Builder &builder, likely_const_ast ast)
{
    if (ast->type == likely_ast_list) {
        if (ast->num_atoms == 0)
            return likely_expression::error(ast, "Empty expression");
        likely_const_ast op = ast->atoms[0];

        // This is an important special case that allows us to have
        // environment variables that aren't wrapped in factory methods.
        if (op->type != likely_ast_list)
            if (const likely_const_env e = lookup(builder.env, op->atom))
                return e->expr->evaluate(builder, ast);

        // Fallback general case
        TRY_EXPR(builder, op, e);
        return e->evaluate(builder, ast);
    } else {
        if (const likely_const_env e = lookup(builder.env, ast->atom)) {
            const_cast<likely_ast>(ast)->type = likely_ast_operator;
            return e->expr->evaluate(builder, ast);
        }

        if ((ast->atom[0] == '"') && (ast->atom[ast->atom_len-1] == '"')) {
            const_cast<likely_ast>(ast)->type = likely_ast_string;
            return new likely_expression(LikelyValue(builder.CreateGlobalStringPtr(string(ast->atom).substr(1, ast->atom_len-2)), likely_i8 | likely_pointer));
        }

        { // Is it a number?
            char *p;
            const double value = strtod(ast->atom, &p);
            if (*p == 0) {
                const_cast<likely_ast>(ast)->type = likely_ast_number;
                return new likely_expression(builder.constant(value, likely_type_from_value(value)));
            }
        }

        { // Is it a type?
            bool ok;
            const likely_type matrixType = likely_type_from_string(ast->atom, &ok);
            if (ok) {
                const_cast<likely_ast>(ast)->type = likely_ast_type;
                return new MatrixType(builder, matrixType);
            }

            const likely_file_type fileType = likely_file_type_from_string(ast->atom, &ok);
            if (ok) {
                const_cast<likely_ast>(ast)->type = likely_ast_type;
                return new likely_expression(LikelyValue(builder.constant(uint64_t(fileType), likely_u32)));
            }
        }

        // Special keyword
        if (!strcmp(ast->atom, "this"))
            return ConstantData::get(builder, likely_retain_env(builder.env));

        const_cast<likely_ast>(ast)->type = likely_ast_invalid;
        return likely_expression::error(ast, "invalid literal");
    }
}

likely_env likely_standard(likely_settings settings, const char *file_name)
{
    const likely_env env = newEnv(RootEnvironment::get());
    env->settings = (likely_settings*) malloc(sizeof(likely_settings));
    memcpy(env->settings, &settings, sizeof(likely_settings));
    if (file_name)
        env->module = new OfflineModule(settings, file_name);
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
    delete env->expr;
    likely_release_ast(env->ast);
    if (env->settings && !env->parent->settings)
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

likely_mat likely_dynamic(likely_vtable vtable, likely_const_mat *mats)
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
        vector<likely_type> types;
        for (size_t i=0; i<vtable->n; i++)
            types.push_back(mats[i]->type);
        vtable->functions.push_back(unique_ptr<JITFunction>(new JITFunction("likely_vtable_entry", unique_ptr<Lambda>(new Lambda(vtable->env, vtable->body, vtable->parameters)).get(), types, false, true)));
        function = vtable->functions.back()->function;
        if (function == NULL)
            return NULL;
    }

    return reinterpret_cast<likely_mat (*)(likely_const_mat const*)>(function)(mats);
}

likely_const_mat likely_result(const struct likely_expression *expr)
{
    if (!expr)
        return NULL;

    if (const likely_const_ast ast = expr->getData())
        return likely_ast_to_string(ast);

    return (likely_const_mat) expr->getData();
}

void *likely_function(const struct likely_expression *expr)
{
    return JITFunction::getFunction(expr);
}

class LazyDefinition : public LikelyOperator
{
    const likely_const_env env;
    const likely_const_ast ast;

    size_t minParameters() const { return 0; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_env env = this->env;
        swap(builder.env, env);
        unique_ptr<const likely_expression> op(get(builder, this->ast));
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

        Builder builder(parent, parent->module, true);
        likely_const_expr expr = NULL;
        env = NULL;
        if (likely_is_definition(statement)) {
            const likely_const_ast rhs = statement->atoms[2];
            if (!strcmp(likely_symbol(rhs), "->")) {
                // Global function
                expr = likely_expression::get(builder, rhs);
            } else {
                // Lazy global value
                expr = new LazyDefinition(builder.env, rhs);
            }
        } else {
            // If `ast` is not a lambda then it is a computation we perform by constructing and executing a parameterless lambda.
            const char *const symbol = ((statement->type == likely_ast_list)
                                        && (statement->num_atoms > 0)
                                        && (statement->atoms[0]->type != likely_ast_list))
                                       ? statement->atoms[0]->atom : "";
            if (Lambda::isFunction(symbol)) {
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
                    likely_assert(tmp != NULL, "evaluation expected a descendant environment.");

                    env = likely_retain_env(evaluated);
                    env->module = parent->module;
                } else {
                    expr = ConstantData::get(data);
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

void likely_shutdown()
{
    likely_release_env(RootEnvironment::get());
    llvm_shutdown();
}
