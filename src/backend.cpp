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
#include <llvm/Support/MD5.h>
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
#include <functional>
#include <future>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <sstream>

#include "likely/backend.h"
#include "likely/io.h"

using namespace llvm;
using namespace std;

namespace {

class LikelyContext
{
    static queue<LikelyContext*> contextPool;
    map<likely_matrix_type, Type*> typeLUT;
    PassManager *PM;

    // use LikelyContext::acquire()
    LikelyContext()
        : PM(new PassManager()), TM(getTargetMachine(false))
    {
        PM->add(createVerifierPass());
        PM->add(new TargetLibraryInfo(Triple(sys::getProcessTriple())));
        PM->add(new DataLayoutPass(*TM->getSubtargetImpl()->getDataLayout()));
        TM->addAnalysisPasses(*PM);
        PassManagerBuilder builder;
        builder.OptLevel = 3;
        builder.SizeLevel = 0;
        builder.LoopVectorize = true;
        builder.Inliner = createAlwaysInlinerPass();
        builder.populateModulePassManager(*PM);
        PM->add(createVerifierPass());
    }

    // use LikelyContext::release()
    ~LikelyContext()
    {
        delete PM;
        delete TM;
    }

public:
    LLVMContext context;
    TargetMachine *TM;

    static LikelyContext *acquire()
    {
        static mutex lock;
        lock_guard<mutex> guard(lock);

        static bool initialized = false;
        if (!initialized) {
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
            initialized = true;
        }

        if (contextPool.empty())
            contextPool.push(new LikelyContext());

        LikelyContext *context = contextPool.front();
        contextPool.pop();
        return context;
    }

    static void release(LikelyContext *context)
    {
        static mutex lock;
        lock_guard<mutex> guard(lock);
        contextPool.push(context);
    }

    static void shutdown()
    {
        while (!contextPool.empty()) {
            delete contextPool.front();
            contextPool.pop();
        }
    }

    IntegerType *nativeInt()
    {
        return Type::getIntNTy(context, unsigned(likely_matrix_native));
    }

    Type *scalar(likely_matrix_type type, bool pointer = false)
    {
        const size_t bits = type & likely_matrix_depth;
        const bool floating = (type & likely_matrix_floating) != 0;
        if (floating) {
            if      (bits == 16) return pointer ? Type::getHalfPtrTy(context)   : Type::getHalfTy(context);
            else if (bits == 32) return pointer ? Type::getFloatPtrTy(context)  : Type::getFloatTy(context);
            else if (bits == 64) return pointer ? Type::getDoublePtrTy(context) : Type::getDoubleTy(context);
        } else {
            if      (bits == 1)  return pointer ? Type::getInt1PtrTy(context)  : (Type*)Type::getInt1Ty(context);
            else if (bits == 8)  return pointer ? Type::getInt8PtrTy(context)  : (Type*)Type::getInt8Ty(context);
            else if (bits == 16) return pointer ? Type::getInt16PtrTy(context) : (Type*)Type::getInt16Ty(context);
            else if (bits == 32) return pointer ? Type::getInt32PtrTy(context) : (Type*)Type::getInt32Ty(context);
            else if (bits == 64) return pointer ? Type::getInt64PtrTy(context) : (Type*)Type::getInt64Ty(context);
        }
        likely_assert(false, "ty invalid matrix bits: %d and floating: %d", bits, floating);
        return NULL;
    }

    Type *toLLVM(likely_matrix_type likely)
    {
        assert(!((likely & likely_matrix_signed) && (likely & likely_matrix_floating)));
        auto result = typeLUT.find(likely);
        if (result != typeLUT.end())
            return result->second;

        Type *llvm;
        if (!(likely & likely_matrix_multi_dimension) && (likely & likely_matrix_depth)) {
            llvm = scalar(likely);
        } else {
            likely_mat str = likely_type_to_string(likely);
            llvm = PointerType::getUnqual(StructType::create(str->data,
                                                             Type::getInt32Ty(context), // ref_count
                                                             Type::getInt32Ty(context), // type
                                                             Type::getInt32Ty(context), // channels
                                                             Type::getInt32Ty(context), // columns
                                                             Type::getInt32Ty(context), // rows
                                                             Type::getInt32Ty(context), // frames
                                                             Type::getInt64Ty(context), // _reserved
                                                             ArrayType::get(Type::getInt8Ty(context), 0), // data
                                                             NULL));
            likely_release_mat(str);
        }

        if (likely & likely_matrix_array)
            llvm = PointerType::getUnqual(llvm);

        typeLUT[likely] = llvm;
        return llvm;
    }

    void optimize(Module &module)
    {
        module.setTargetTriple(sys::getProcessTriple());
//        DebugFlag = true;
        PM->run(module);
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
queue<LikelyContext*> LikelyContext::contextPool;

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

static likely_env newEnv(likely_const_env parent)
{
    likely_env env = (likely_env) malloc(sizeof(likely_environment));
    if (!env)
        return NULL;

    env->type = likely_environment_void;
    if (parent) {
        if (parent->type & likely_environment_offline      ) env->type |= likely_environment_offline;
        if (parent->type & likely_environment_parallel     ) env->type |= likely_environment_parallel;
        if (parent->type & likely_environment_heterogeneous) env->type |= likely_environment_heterogeneous;
    }
    env->parent = likely_retain_env(parent);
    env->ast = NULL;
    env->module = parent ? parent->module : NULL;
    env->value = NULL;
    env->ref_count = 1;
    return env;
}

struct Builder;

} // namespace (anonymous)

typedef struct likely_expression *likely_expr;
typedef struct likely_expression const *likely_const_expr;

struct likely_expression
{
    Value *value;
    likely_matrix_type type;

    likely_expression(Value *value = NULL, likely_matrix_type type = likely_matrix_void, likely_const_mat data = NULL)
        : value(value), type(type), data(data)
    {
        if (value && type) {
            // Check type correctness
            likely_assert(!(type & likely_matrix_floating) || !(type & likely_matrix_signed), "type can't be both floating and signed (integer)");
            likely_matrix_type inferred = toLikely(value->getType());
            if (!(inferred & likely_matrix_multi_dimension)) {
                // Can't represent these flags in LLVM IR for scalar types
                if (type & likely_matrix_signed)
                    inferred |= likely_matrix_signed;
                if (type & likely_matrix_saturated)
                    inferred |= likely_matrix_saturated;
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

    virtual ~likely_expression()
    {
        likely_release_mat(data);
    }

    virtual int uid() const { return 0; }
    virtual size_t maxParameters() const { return 0; }
    virtual size_t minParameters() const { return maxParameters(); }
    virtual const char *symbol() const { return ""; }

    virtual likely_const_mat getData() const
    {
        if (data || !value)
            return data;

        if (ConstantInt *constantInt = dyn_cast<ConstantInt>(value)) {
            data = likely_new(type, 1, 1, 1, 1, NULL);
            likely_set_element(const_cast<likely_mat>(data), (type & likely_matrix_signed) ? double(constantInt->getSExtValue())
                                                                                           : double(constantInt->getZExtValue()), 0, 0, 0, 0);
        } else if (ConstantFP *constantFP = dyn_cast<ConstantFP>(value)) {
            const APFloat &apFloat = constantFP->getValueAPF();
            if ((&apFloat.getSemantics() == &APFloat::IEEEsingle) || (&apFloat.getSemantics() == &APFloat::IEEEdouble)) {
                // This should always be the case
                data = likely_new(type, 1, 1, 1, 1, NULL);
                likely_set_element(const_cast<likely_mat>(data), &apFloat.getSemantics() == &APFloat::IEEEsingle ? double(apFloat.convertToFloat())
                                                                                                                 : apFloat.convertToDouble(), 0, 0, 0, 0);

            }
        } else if (GEPOperator *gepOperator = dyn_cast<GEPOperator>(value)) {
            if (GlobalValue *globalValue = dyn_cast<GlobalValue>(gepOperator->getPointerOperand()))
                if (ConstantDataSequential *constantDataSequential = dyn_cast<ConstantDataSequential>(globalValue->getOperand(0)))
                    if (constantDataSequential->isCString())
                        data = likely_string(constantDataSequential->getAsCString().data());
        }

        return data;
    }

    void setData(likely_const_mat data) const
    {
        assert(!getData());
        this->data = data;
    }

    virtual likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const;

    operator Value*() const { return value; }
    operator likely_matrix_type() const { return type; }

    void dump() const
    {
        likely_const_mat m = likely_type_to_string(type);
        cerr << m->data << " ";
        value->dump();
        likely_release_mat(m);
    }

    static likely_const_expr error(likely_const_ast ast, const char *message)
    {
        likely_throw(ast, message);
        return NULL;
    }

    static likely_const_expr lookup(likely_const_env env, const char *name)
    {
        if (!env)
            return NULL;
        if ((env->type & likely_environment_definition) && !strcmp(name, likely_symbol(env->ast)))
            return env->value;
        return lookup(env->parent, name);
    }

    static void define(likely_env &env, const char *name, likely_const_expr value)
    {
        assert(name && strcmp(name, ""));
        env = newEnv(env);
        env->type |= likely_environment_definition;
        env->ast = likely_atom(name, uint32_t(strlen(name)));
        env->value = value;
    }

    static likely_const_expr undefine(likely_env &env, const char *name)
    {
        assert(env->type & likely_environment_definition);
        likely_assert(!strcmp(name, likely_symbol(env->ast)), "undefine variable mismatch");
        likely_const_expr value = env->value;
        env->value = NULL;
        likely_env old = env;
        env = const_cast<likely_env>(env->parent);
        likely_release_env(old);
        return value;
    }

    static size_t length(likely_const_ast ast)
    {
        return ast ? ((ast->type == likely_ast_list) ? ast->num_atoms : 1) : 0;
    }

    static bool isMat(Type *type)
    {
        // This is safe because matricies are the only struct types created by the backend
        if (PointerType *ptr = dyn_cast<PointerType>(type))
            if (dyn_cast<StructType>(ptr->getElementType()))
                return true;
        return false;
    }

    static likely_matrix_type toLikely(Type *llvm)
    {
        if      (llvm->isIntegerTy()) return llvm->getIntegerBitWidth();
        else if (llvm->isHalfTy())    return likely_matrix_f16;
        else if (llvm->isFloatTy())   return likely_matrix_f32;
        else if (llvm->isDoubleTy())  return likely_matrix_f64;
        else {
            if (FunctionType *function = dyn_cast<FunctionType>(llvm)) {
                return toLikely(function->getReturnType());
            } else {
                if (Type *element = dyn_cast<PointerType>(llvm)->getElementType()) {
                    if (StructType *matrix = dyn_cast<StructType>(element)) {
                        return likely_type_from_string(matrix->getName().str().c_str(), NULL);
                    } else {
                        likely_matrix_type type = toLikely(element);
                        if (!isa<FunctionType>(element))
                            type |= likely_matrix_array;
                        return type;
                    }
                } else {
                    return likely_matrix_void;
                }
            }
        }
    }

private:
    mutable likely_const_mat data; // use getData() and setData()
};

struct likely_module
{
    LikelyContext *context;
    Module *module;
    vector<likely_const_expr> exprs;
    vector<likely_const_mat> mats;

    likely_module()
        : context(LikelyContext::acquire())
        , module(new Module("likely_module", context->context)) {}

    virtual ~likely_module()
    {
        finalize();
        for (likely_const_expr expr : exprs)
            delete expr;
        for (likely_const_mat mat : mats)
            likely_release_mat(mat);
    }

    void optimize()
    {
        context->optimize(*module);
    }

    void finalize()
    {
        if (module) {
            delete module;
            module = NULL;
        }
        if (context) {
            LikelyContext::release(context);
            context = NULL;
        }
    }
};

namespace {

class OfflineModule : public likely_module
{
    const string fileName;

public:
    OfflineModule(const string &fileName)
        : fileName(fileName) {}

    ~OfflineModule()
    {
        error_code errorCode;
        tool_output_file output(fileName.c_str(), errorCode, sys::fs::F_None);
        likely_assert(!errorCode, "%s", errorCode.message().c_str());

        const string extension = fileName.substr(fileName.find_last_of(".") + 1);
        if (extension == "ll") {
            module->print(output.os(), NULL);
        } else if (extension == "bc") {
            WriteBitcodeToFile(module, output.os());
        } else {
            optimize();
            PassManager pm;
            formatted_raw_ostream fos(output.os());
            context->TM->addPassesToEmitFile(pm, fos, extension == "s" ? TargetMachine::CGFT_AssemblyFile : TargetMachine::CGFT_ObjectFile);
            pm.run(*module);
        }

        output.keep();
    }
};

struct Builder : public IRBuilder<>
{
    likely_env env;

    Builder(likely_env env)
        : IRBuilder<>(env->module ? env->module->context->context : getGlobalContext()), env(env) {}

    likely_expression constant(uint64_t value, likely_matrix_type type = likely_matrix_native)
    {
        const unsigned depth = unsigned(type & likely_matrix_depth);
        return likely_expression(Constant::getIntegerValue(Type::getIntNTy(getContext(), depth), APInt(depth, value)), type);
    }

    likely_expression constant(double value, likely_matrix_type type)
    {
        const size_t depth = type & likely_matrix_depth;
        if (type & likely_matrix_floating) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return likely_expression(ConstantFP::get(Type::getDoubleTy(getContext()), value), type);
            else if (depth == 32) return likely_expression(ConstantFP::get(Type::getFloatTy(getContext()), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return likely_expression(NULL, likely_matrix_void); }
        } else {
            return constant(uint64_t(value), type);
        }
    }

    likely_expression zero(likely_matrix_type type = likely_matrix_native) { return constant(0.0, type); }
    likely_expression one (likely_matrix_type type = likely_matrix_native) { return constant(1.0, type); }
    likely_expression intMax(likely_matrix_type type) { const likely_matrix_type bits = type & likely_matrix_depth; return constant((uint64_t) (1 << (bits - ((type & likely_matrix_signed) ? 1 : 0)))-1, bits); }
    likely_expression intMin(likely_matrix_type type) { const likely_matrix_type bits = type & likely_matrix_depth; return constant((uint64_t) ((type & likely_matrix_signed) ? (1 << (bits - 1)) : 0), bits); }
    likely_expression nullMat() { return likely_expression(ConstantPointerNull::get(::cast<PointerType>((Type*)multiDimension())), likely_matrix_multi_dimension); }
    likely_expression nullData() { return likely_expression(ConstantPointerNull::get(Type::getInt8PtrTy(getContext())), likely_matrix_u8 | likely_matrix_array); }

    likely_expression channels(likely_const_expr m) { return (m && (*m & likely_matrix_multi_channel)) ? cast(likely_expression(CreateLoad(CreateStructGEP(*m, 2), "channels"), likely_matrix_u32), likely_matrix_native) : one(); }
    likely_expression columns (likely_const_expr m) { return (m && (*m & likely_matrix_multi_column )) ? cast(likely_expression(CreateLoad(CreateStructGEP(*m, 3), "columns" ), likely_matrix_u32), likely_matrix_native) : one(); }
    likely_expression rows    (likely_const_expr m) { return (m && (*m & likely_matrix_multi_row    )) ? cast(likely_expression(CreateLoad(CreateStructGEP(*m, 4), "rows"    ), likely_matrix_u32), likely_matrix_native) : one(); }
    likely_expression frames  (likely_const_expr m) { return (m && (*m & likely_matrix_multi_frame  )) ? cast(likely_expression(CreateLoad(CreateStructGEP(*m, 5), "frames"  ), likely_matrix_u32), likely_matrix_native) : one(); }
    Value *data    (Value *value, likely_matrix_type type) { return CreatePointerCast(CreateStructGEP(value, 6), env->module->context->scalar(type, true)); }
    likely_expression data    (likely_const_expr m) { return likely_expression(data(m->value, m->type), (*m & likely_matrix_element) | likely_matrix_array); }

    void steps(likely_const_expr matrix, Value *channelStep, Value **columnStep, Value **rowStep, Value **frameStep)
    {
        *columnStep = CreateMul(channels(matrix), channelStep, "x_step");
        *rowStep    = CreateMul(columns(matrix), *columnStep, "y_step");
        *frameStep  = CreateMul(rows(matrix), *rowStep, "t_step");
    }

    likely_expression cast(const likely_expression &x, likely_matrix_type type)
    {
        type &= likely_matrix_element;
        if ((x.type & likely_matrix_element) == type)
            return likely_expression(x, type);
        if ((type & likely_matrix_depth) == 0) {
            type |= x.type & likely_matrix_depth;
            if (type & likely_matrix_floating)
                type = likely_type_from_types(type, likely_matrix_floating);
        }
        Type *dstType = env->module->context->scalar(type);
        return likely_expression(CreateCast(CastInst::getCastOpcode(x, (x & likely_matrix_signed) != 0, dstType, (type & likely_matrix_signed) != 0), x, dstType), type);
    }

    likely_const_expr lookup(const char *name) const { return likely_expression::lookup(env, name); }
    void define(const char *name, likely_const_expr e) { likely_expression::define(env, name, e); }
    likely_const_expr undefine(const char *name) { return likely_expression::undefine(env, name); }

    void undefineAll(likely_const_ast args, bool deleteExpression)
    {
        if (!args)
            return;

        if (args->type == likely_ast_list) {
            for (size_t i=0; i<args->num_atoms; i++) {
                likely_const_expr expression = undefine(args->atoms[args->num_atoms-i-1]->atom);
                if (deleteExpression) delete expression;
            }
        } else {
            likely_const_expr expression = undefine(args->atom);
            if (deleteExpression) delete expression;
        }
    }

    struct ConstantMat : public likely_expression
    {
        likely_module *mod;
        ConstantMat(Builder &builder, likely_const_mat m)
            : likely_expression(ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(likely_mat)), uintptr_t(m)), builder.toLLVM(m->type)), m->type, m)
            , mod(builder.env->module) {}

        ~ConstantMat()
        {
            if (value->getNumUses() > 0)
                mod->mats.push_back(likely_retain_mat(getData()));
        }
    };

    likely_const_expr mat(likely_const_mat data)
    {
        if (!data)
            return NULL;
        else if (!(data->type & likely_matrix_multi_dimension))
            return new likely_expression(constant(likely_element(data, 0, 0, 0, 0), data->type), data->type, data);
        else
            return new ConstantMat(*this, data);
    }

    IntegerType *nativeInt() { return env->module->context->nativeInt(); }
    Type *multiDimension() { return toLLVM(likely_matrix_multi_dimension); }
    Module *module() { return env->module->module; }
    Type *toLLVM(likely_matrix_type likely) { return env->module->context->toLLVM(likely); }

    likely_const_expr expression(likely_const_ast ast);

    likely_expression toMat(likely_const_expr expr)
    {
        if (likely_expression::isMat(expr->value->getType()))
            return likely_expression(expr->value, expr->type);

        if (expr->value->getType()->isPointerTy() /* assume it's a string for now */) {
            Function *likelyString = module()->getFunction("likely_string");
            if (!likelyString) {
                FunctionType *functionType = FunctionType::get(toLLVM(likely_matrix_string), Type::getInt8PtrTy(getContext()), false);
                likelyString = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_string", module());
                likelyString->setCallingConv(CallingConv::C);
                likelyString->setDoesNotAlias(0);
                likelyString->setDoesNotAlias(1);
                likelyString->setDoesNotCapture(1);
                sys::DynamicLibrary::AddSymbol("likely_string", (void*) likely_string);
            }
            return likely_expression(CreateCall(likelyString, *expr), likely_matrix_string);
        }

        Function *likelyScalar = module()->getFunction("likely_scalar_va");
        if (!likelyScalar) {
            Type *params[] = { Type::getInt32Ty(getContext()), Type::getDoubleTy(getContext()) };
            FunctionType *functionType = FunctionType::get(multiDimension(), params, true);
            likelyScalar = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_scalar_va", module());
            likelyScalar->setCallingConv(CallingConv::C);
            likelyScalar->setDoesNotAlias(0);
            sys::DynamicLibrary::AddSymbol("likely_scalar_va", (void*) likely_scalar_va);
            sys::DynamicLibrary::AddSymbol("lle_X_likely_scalar_va", (void*) lle_X_likely_scalar_va);
        }

        vector<Value*> args;
        args.push_back(constant(uint64_t(expr->type), likely_matrix_u32));
        args.push_back(cast(*expr, likely_matrix_f64));
        args.push_back(ConstantFP::get(getContext(), APFloat::getNaN(APFloat::IEEEdouble)));
        return likely_expression(CreateCall(likelyScalar, args), likely_matrix_multi_dimension);
    }

    likely_expression newMat(Value *type, Value *channels, Value *columns, Value *rows, Value *frames, Value *data)
    {
        Function *likelyNew = module()->getFunction("likely_new");
        if (!likelyNew) {
            Type *params[] = { Type::getInt32Ty(getContext()), Type::getInt32Ty(getContext()), Type::getInt32Ty(getContext()), Type::getInt32Ty(getContext()), Type::getInt32Ty(getContext()), Type::getInt8PtrTy(getContext()) };
            FunctionType *functionType = FunctionType::get(multiDimension(), params, false);
            likelyNew = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_new", module());
            likelyNew->setCallingConv(CallingConv::C);
            likelyNew->setDoesNotAlias(0);
            likelyNew->setDoesNotAlias(6);
            likelyNew->setDoesNotCapture(6);
            sys::DynamicLibrary::AddSymbol("likely_new", (void*) likely_new);
        }
        Value* args[] = { type, channels, columns, rows, frames, data };
        return likely_expression(CreateCall(likelyNew, args), likely_matrix_multi_dimension);
    }

    likely_expression retainMat(Value *m)
    {
        Function *likelyRetain = module()->getFunction("likely_retain_mat");
        if (!likelyRetain) {
            FunctionType *functionType = FunctionType::get(multiDimension(), multiDimension(), false);
            likelyRetain = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_retain_mat", module());
            likelyRetain->setCallingConv(CallingConv::C);
            likelyRetain->setDoesNotAlias(1);
            likelyRetain->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("likely_retain_mat", (void*) likely_retain_mat);
        }
        return likely_expression(CreateCall(likelyRetain, CreatePointerCast(m, multiDimension())), likely_matrix_multi_dimension);
    }

    likely_expression releaseMat(Value *m)
    {
        Function *likelyRelease = module()->getFunction("likely_release_mat");
        if (!likelyRelease) {
            FunctionType *functionType = FunctionType::get(Type::getVoidTy(getContext()), multiDimension(), false);
            likelyRelease = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_release_mat", module());
            likelyRelease->setCallingConv(CallingConv::C);
            likelyRelease->setDoesNotAlias(1);
            likelyRelease->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("likely_release_mat", (void*) likely_release_mat);
        }
        return likely_expression(CreateCall(likelyRelease, CreatePointerCast(m, multiDimension())), likely_matrix_void);
    }

private:
    static GenericValue lle_X_likely_scalar_va(FunctionType *, const vector<GenericValue> &Args)
    {
        vector<double> values;
        for (size_t i=1; i<Args.size()-1; i++)
            values.push_back(Args[i].DoubleVal);
        return GenericValue(likely_scalar(likely_matrix_type(Args[0].IntVal.getZExtValue()), values.data(), uint32_t(values.size())));
    }
};

#define TRY_EXPR(BUILDER, AST, EXPR)                                       \
const unique_ptr<const likely_expression> EXPR((BUILDER).expression(AST)); \
if (!EXPR.get()) return NULL;                                              \

struct Symbol : public likely_expression
{
    string name;
    vector<likely_matrix_type> parameters;

    Symbol(likely_const_expr function = NULL) { associateFunction(function); }

    void associateFunction(likely_const_expr expr)
    {
        if (!expr)
            return;

        Function *function = cast<Function>(expr->value);
        name = function->getName();
        parameters.clear();
        for (const Argument &argument : function->getArgumentList())
            parameters.push_back(toLikely(argument.getType()));
        value = function;
        type = toLikely(function->getReturnType());
        setData(likely_retain_mat(expr->getData()));
    }

private:
    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (parameters.size() != ((ast->type == likely_ast_list) ? ast->num_atoms-1 : 0))
            return error(ast, "incorrect argument count");

        Function *symbol = builder.module()->getFunction(name);
        if (!symbol) {
            // Translate definition type across contexts
            vector<Type*> llvmParameters;
            for (likely_matrix_type parameter : parameters)
                llvmParameters.push_back(builder.toLLVM(parameter));
            FunctionType *functionType = FunctionType::get(builder.toLLVM(type), llvmParameters, false);
            symbol = Function::Create(functionType, GlobalValue::ExternalLinkage, name, builder.module());
        }

        vector<Value*> args;
        if (ast->type == likely_ast_list)
            for (size_t i=1; i<ast->num_atoms; i++) {
                TRY_EXPR(builder, ast->atoms[i], arg)
                args.push_back(builder.cast(*arg.get(), parameters[i-1]));
            }

        return new likely_expression(builder.CreateCall(symbol, args), type);
    }
};

struct Lambda;

struct JITFunction : public Symbol
{
    void *function = NULL;
    ExecutionEngine *EE = NULL;
    likely_env env;

    JITFunction(const string &name, const Lambda *lambda, likely_const_env parent, const vector<likely_matrix_type> &parameters, bool abandon, bool interpreter, bool arrayCC);

    ~JITFunction()
    {
        if (EE && env->module->module) // interpreter
            EE->removeModule(env->module->module);
        delete EE;
        likely_release_env(env);
    }

private:
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

likely_const_expr likely_expression::evaluate(Builder &, likely_const_ast) const
{
    return new likely_expression(value, type);
}

struct likely_virtual_table : public LikelyOperator
{
    const likely_const_env env;
    const likely_const_ast body, parameters;
    const size_t n;
    vector<unique_ptr<JITFunction>> functions;

    likely_virtual_table(likely_const_env env, likely_const_ast body, likely_const_ast parameters)
        : env(env), body(body), parameters(parameters), n(length(parameters)) {}

private:
    likely_const_expr evaluateOperator(Builder &, likely_const_ast) const { return NULL; }
};

namespace {

struct RootEnvironment
{
    // Provide public access to an environment that includes the standard library.
    static likely_env get()
    {
        static bool init = false;
        if (!init) {
            likely_ast ast = likely_lex_and_parse(likely_standard_library, likely_source_gfm);
            builtins() = likely_repl(ast, builtins(), NULL, NULL);
            likely_release_ast(ast);
            init = true;
        }

        return builtins();
    }

protected:
    // Provide protected access for registering builtins.
    static likely_env &builtins()
    {
        static likely_env root = newEnv(NULL);
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
    likely_matrix_type t;
    MatrixType(Builder &builder, likely_matrix_type t)
        : t(t)
    {
        value = builder.constant(uint64_t(t), likely_matrix_u32);
        type = likely_matrix_u32;
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
            return new likely_expression(*this);
        }
    }
};

// As a special exception, this function is allowed to set ast->type
likely_const_expr Builder::expression(likely_const_ast ast)
{
    if (ast->type == likely_ast_list) {
        if (ast->num_atoms == 0)
            return likely_expression::error(ast, "Empty expression");
        likely_const_ast op = ast->atoms[0];
        if (op->type != likely_ast_list)
            if (likely_const_expr e = lookup(op->atom))
                return e->evaluate(*this, ast);
        TRY_EXPR(*this, op, e);
        return e->evaluate(*this, ast);
    } else {
        if (likely_const_expr e = lookup(ast->atom)) {
            const_cast<likely_ast>(ast)->type = likely_ast_operator;
            return e->evaluate(*this, ast);
        }

        if ((ast->atom[0] == '"') && (ast->atom[ast->atom_len-1] == '"')) {
            const_cast<likely_ast>(ast)->type = likely_ast_string;
            return new likely_expression(CreateGlobalStringPtr(string(ast->atom).substr(1, ast->atom_len-2)), likely_matrix_i8 | likely_matrix_array);
        }

        { // Is it a number?
            char *p;
            const double value = strtod(ast->atom, &p);
            if (*p == 0) {
                const_cast<likely_ast>(ast)->type = likely_ast_number;
                return new likely_expression(constant(value, likely_type_from_value(value)));
            }
        }

        { // Is it a type?
            bool ok;
            likely_matrix_type type = likely_type_from_string(ast->atom, &ok);
            if (ok) {
                const_cast<likely_ast>(ast)->type = likely_ast_type;
                return new MatrixType(*this, type);
            }
        }

        const_cast<likely_ast>(ast)->type = likely_ast_invalid;
        return likely_expression::error(ast, "invalid literal");
    }
}

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
            return new likely_expression(builder.AXIS(expr.get()));                  \
        }                                                                            \
    }                                                                                \
};                                                                                   \
LIKELY_REGISTER(AXIS)                                                                \

LIKELY_REGISTER_AXIS(channels)
LIKELY_REGISTER_AXIS(columns)
LIKELY_REGISTER_AXIS(rows)
LIKELY_REGISTER_AXIS(frames)

class notExpression : public SimpleUnaryOperator
{
    const char *symbol() const { return "~"; }
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(builder.CreateXor(builder.intMax(*arg), arg->value), *arg);
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
        likely_expression xc(builder.cast(*x.get(), likely_type_from_types(*x, likely_matrix_floating)));
        return new likely_expression(builder.CreateCall(Intrinsic::getDeclaration(builder.module(), id(), xc.value->getType()), xc), xc);
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
        likely_matrix_type type = likely_type_from_types(*lhs, *rhs);
        return evaluateArithmetic(builder, builder.cast(*lhs.get(), type), builder.cast(*rhs.get(), type));
    }
    virtual likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const = 0;
};

class SimpleArithmeticOperator : public ArithmeticOperator
{
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        // Fold constant expressions
        if (likely_const_mat LHS = lhs.getData()) {
            if (likely_const_mat RHS = rhs.getData()) {
                static map<const char*, likely_const_env> envLUT;
                static map<const char*, void*> functionLUT;
                static mutex lock;

                lock.lock();
                auto function = functionLUT.find(symbol());
                if (function == functionLUT.end()) {
                    const string code = string("(a b) :-> { dst := a.imitate (dst a b) :=> (") + symbol() + string(" a b) }");
                    likely_const_ast ast = likely_lex_and_parse(code.c_str(), likely_source_lisp);
                    likely_env parent = likely_jit();
                    likely_env env = likely_eval(ast->atoms[0], parent);
                    void *f = likely_compile(env, NULL, 0);
                    likely_release_env(parent);
                    likely_release_ast(ast);
                    envLUT.insert(pair<const char*, likely_const_env>(symbol(), env));
                    functionLUT.insert(pair<const char*, void*>(symbol(), f));
                    function = functionLUT.find(symbol());
                }
                lock.unlock();

                return builder.mat(reinterpret_cast<likely_mat (*)(likely_const_mat, likely_const_mat)>(function->second)(LHS, RHS));
            }
        }

        return new likely_expression(evaluateSimpleArithmetic(builder, lhs, rhs), lhs);
    }
    virtual Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const = 0;
};

class addExpression : public SimpleArithmeticOperator
{
    const char *symbol() const { return "+"; }
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        if (lhs.type & likely_matrix_floating) {
            return builder.CreateFAdd(lhs, rhs);
        } else {
            if (lhs.type & likely_matrix_saturated) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), (lhs.type & likely_matrix_signed) ? Intrinsic::sadd_with_overflow : Intrinsic::uadd_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = (lhs.type & likely_matrix_signed) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, builder.zero(lhs)), builder.intMax(lhs), builder.intMin(lhs)) : builder.intMax(lhs).value;
                return builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0));
            } else {
                return builder.CreateAdd(lhs, rhs);
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
        expr1.reset(builder.expression(ast->atoms[1]));
        if (!expr1.get())
            return NULL;

        if (ast->num_atoms == 2) {
            // Unary negation
            expr2.reset(new likely_expression(builder.zero(*expr1)));
            expr2.swap(expr1);
        } else {
            // Binary subtraction
            expr2.reset(builder.expression(ast->atoms[2]));
            if (!expr2.get())
                return NULL;
        }

        likely_matrix_type type = likely_type_from_types(*expr1, *expr2);
        likely_expression lhs = builder.cast(*expr1.get(), type);
        likely_expression rhs = builder.cast(*expr2.get(), type);

        if (type & likely_matrix_floating) {
            return new likely_expression(builder.CreateFSub(lhs, rhs), type);
        } else {
            if (type & likely_matrix_saturated) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), (lhs.type & likely_matrix_signed) ? Intrinsic::ssub_with_overflow : Intrinsic::usub_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = (lhs.type & likely_matrix_signed) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, builder.zero(lhs)), builder.intMax(lhs), builder.intMin(lhs)) : builder.intMin(lhs).value;
                return new likely_expression(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), type);
            } else {
                return new likely_expression(builder.CreateSub(lhs, rhs), type);
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
        if (lhs & likely_matrix_floating) {
            return builder.CreateFMul(lhs, rhs);
        } else {
            if (lhs.type & likely_matrix_saturated) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), (lhs.type & likely_matrix_signed) ? Intrinsic::smul_with_overflow : Intrinsic::umul_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *zero = builder.zero(lhs);
                Value *overflowResult = (lhs.type & likely_matrix_signed) ? builder.CreateSelect(builder.CreateXor(builder.CreateICmpSGE(lhs, zero), builder.CreateICmpSGE(rhs, zero)), builder.intMin(lhs), builder.intMax(lhs)) : builder.intMax(lhs).value;
                return builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0));
            } else {
                return builder.CreateMul(lhs, rhs);
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
        if (n.type & likely_matrix_floating) {
            return builder.CreateFDiv(n, d);
        } else {
            if (n.type & likely_matrix_signed) {
                if (n.type & likely_matrix_saturated) {
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
        return (lhs.type & likely_matrix_floating) ? builder.CreateFRem(lhs, rhs)
                                                   : ((lhs.type & likely_matrix_signed) ? builder.CreateSRem(lhs, rhs)
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
        return (lhs.type & likely_matrix_signed) ? builder.CreateAShr(lhs, rhs.value) : builder.CreateLShr(lhs, rhs.value);
    }
};
LIKELY_REGISTER(shiftRight)

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                                                            \
class OP##Expression : public ArithmeticOperator                                                                                                                       \
{                                                                                                                                                                      \
    const char *symbol() const { return #SYM; }                                                                                                                        \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const                                           \
    {                                                                                                                                                                  \
        return new likely_expression((lhs.type & likely_matrix_floating) ? builder.CreateFCmpO##OP(lhs, rhs)                                                           \
                                                                         : ((lhs.type & likely_matrix_signed) ? builder.CreateICmpS##OP(lhs, rhs)                      \
                                                                                                              : builder.CreateICmpU##OP(lhs, rhs)), likely_matrix_u1); \
    }                                                                                                                                                                  \
};                                                                                                                                                                     \
LIKELY_REGISTER(OP)                                                                                                                                                    \

LIKELY_REGISTER_COMPARISON(LT, <)
LIKELY_REGISTER_COMPARISON(LE, <=)
LIKELY_REGISTER_COMPARISON(GT, >)
LIKELY_REGISTER_COMPARISON(GE, >=)

#define LIKELY_REGISTER_EQUALITY(OP, SYM)                                                                                       \
class OP##Expression : public ArithmeticOperator                                                                                \
{                                                                                                                               \
    const char *symbol() const { return #SYM; }                                                                                 \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const    \
    {                                                                                                                           \
        return new likely_expression((lhs.type & likely_matrix_floating) ? builder.CreateFCmpO##OP(lhs, rhs)                    \
                                                                         : builder.CreateICmp##OP(lhs, rhs), likely_matrix_u1); \
    }                                                                                                                           \
};                                                                                                                              \
LIKELY_REGISTER(OP)                                                                                                             \

LIKELY_REGISTER_EQUALITY(EQ, ==)
LIKELY_REGISTER_EQUALITY(NE, !=)

class BinaryMathOperator : public SimpleBinaryOperator
{
    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &x, const unique_ptr<const likely_expression> &n) const
    {
        const likely_matrix_type type = likely_type_from_types(likely_type_from_types(*x, *n), likely_matrix_floating);
        const likely_expression xc(builder.cast(*x.get(), type));
        const likely_expression nc(builder.cast(*n.get(), type));
        return new likely_expression(builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), id(), xc.value->getType()), xc, nc), xc);
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
        if (likely_env env = likely_eval(ast->atoms[1], builder.env)) {
            if (env->type & likely_environment_definition) swap(env->value, value);
            else                                           value = builder.mat(likely_retain_mat(likely_result(env)));
            likely_release_env(env);
        }

        if (!value)
            value = builder.expression(ast->atoms[2]);
        return value;
    }
};
LIKELY_REGISTER(try)

struct Lambda : public LikelyOperator
{
    likely_const_ast body, parameters;

    Lambda(likely_const_ast body, likely_const_ast parameters = NULL)
        : body(body), parameters(parameters) {}

    ~Lambda()
    {
        for (JITFunction *jitFunction : jitFunctions)
            delete jitFunction;
    }

    likely_const_expr generate(Builder &builder, vector<likely_matrix_type> parameters, string name, bool arrayCC, bool returnConstantOrMatrix) const
    {
        while (parameters.size() < maxParameters())
            parameters.push_back(likely_matrix_multi_dimension);

        vector<Type*> llvmTypes;
        if (arrayCC) {
            // Array calling convention - All arguments (which must be matrix pointers) come stored in an array.
            llvmTypes.push_back(PointerType::get(builder.multiDimension(), 0));
        } else {
            for (const likely_matrix_type &parameter : parameters)
                llvmTypes.push_back(builder.toLLVM(parameter));
        }

        BasicBlock *originalInsertBlock = builder.GetInsertBlock();
        Function *tmpFunction = cast<Function>(builder.module()->getOrInsertFunction(name+"_tmp", FunctionType::get(Type::getVoidTy(builder.getContext()), llvmTypes, false)));
        BasicBlock *entry = BasicBlock::Create(builder.getContext(), "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        vector<likely_const_expr> arguments;
        if (arrayCC) {
            Value *argumentArray = tmpFunction->arg_begin();
            for (size_t i=0; i<parameters.size(); i++) {
                Value *load = builder.CreateLoad(builder.CreateGEP(argumentArray, builder.constant(i)));
                if (parameters[i] & likely_matrix_multi_dimension)
                    load = builder.CreatePointerCast(load, builder.toLLVM(parameters[i]));
                else
                    load = builder.CreateLoad(builder.CreateGEP(builder.data(load, parameters[i]), builder.zero()));
                arguments.push_back(new likely_expression(load, parameters[i]));
            }
        } else {
            Function::arg_iterator it = tmpFunction->arg_begin();
            size_t i = 0;
            while (it != tmpFunction->arg_end())
                arguments.push_back(new likely_expression(it++, parameters[i++]));
        }

        for (size_t i=0; i<arguments.size(); i++) {
            stringstream name; name << "arg_" << i;
            arguments[i]->value->setName(name.str());
        }

        unique_ptr<const likely_expression> result(evaluateFunction(builder, arguments));
        for (likely_const_expr arg : arguments)
            delete arg;
        if (!result)
            return NULL;

        // If we are expecting a constant or a matrix and don't get one then make a matrix
        if (returnConstantOrMatrix && !result->getData() && !isMat(result->value->getType()))
            result.reset(new likely_expression(builder.toMat(result.get())));

        // If we are returning a constant matrix, make sure to retain a copy
        if (isa<ConstantExpr>(result->value) && isMat(result->value->getType()))
            result.reset(new likely_expression(builder.CreatePointerCast(builder.retainMat(result->value), builder.toLLVM(result->type)), result->type, likely_retain_mat(result->getData())));

        builder.CreateRet(*result);

        Function *function = cast<Function>(builder.module()->getOrInsertFunction(name, FunctionType::get(result->value->getType(), llvmTypes, false)));

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
        return new likely_expression(function, likely_matrix_void, likely_retain_mat(result->getData()));
    }

    likely_mat evaluateConstantFunction(likely_const_env env, const vector<likely_const_mat> &args) const
    {
        vector<likely_matrix_type> params;
        for (likely_const_mat arg : args)
            params.push_back(arg->type);

        JITFunction jit("likely_jit_function", this, env, params, false, true, !args.empty());
        if (jit.function) { // compiler
            return args.empty() ? reinterpret_cast<likely_mat (*)()>(jit.function)()
                                : reinterpret_cast<likely_mat (*)(likely_const_mat const*)>(jit.function)(args.data());
        } else if (jit.EE) { // interpreter
            vector<GenericValue> gv;
            if (!args.empty())
                gv.push_back(GenericValue((void*) args.data()));
            return (likely_mat) jit.EE->runFunction(cast<Function>(jit.value), gv).PointerVal;
        } else { // constant or error
            return likely_retain_mat(jit.getData());
        }
    }

    static likely_const_mat getResult(likely_const_env env)
    {
        if (!env || !env->value)
            return NULL;
        if (likely_const_mat m = env->value->getData())
            return m;
        if (env->value->uid() == UID()) {
            const Lambda *lambda = static_cast<const Lambda*>(env->value);
            if (lambda->maxParameters() == 0) {
                likely_const_mat m = lambda->evaluateConstantFunction(env, vector<likely_const_mat>());
                lambda->setData(m);
                return m;
            }
        }
        return NULL;
    }

    static void *getFunction(likely_const_env env, const vector<likely_matrix_type> &types)
    {
        if (!env || !env->value || (env->value->uid() != UID()))
            return NULL;
        const Lambda *lambda = static_cast<const Lambda*>(env->value);
        JITFunction *jitFunction = new JITFunction("likely_jit_function", lambda, env, types, true, false, false);
        lambda->jitFunctions.push_back(jitFunction);
        return jitFunction->function;
    }

private:
    mutable vector<JITFunction*> jitFunctions;

    static int UID() { return __LINE__; }
    int uid() const { return UID(); }
    size_t maxParameters() const { return length(parameters); }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_expr result = NULL;

        vector<likely_const_expr> args;
        vector<likely_const_mat> constantArgs;
        const size_t arguments = length(ast)-1;
        for (size_t i=0; i<arguments; i++) {
            likely_const_expr arg = builder.expression(ast->atoms[i+1]);
            if (!arg)
                goto cleanup;

            if (constantArgs.size() == args.size())
                if (likely_const_mat constantArg = arg->getData())
                    constantArgs.push_back(constantArg);

            args.push_back(arg);
        }

        result = (constantArgs.size() == args.size()) ? builder.mat(evaluateConstantFunction(builder.env, constantArgs))
                                                      : evaluateFunction(builder, args);

    cleanup:
        for (likely_const_expr arg : args)
            delete arg;
        return result;
    }

    likely_const_expr evaluateFunction(Builder &builder, const vector<likely_const_expr> &args) const
    {
        assert(args.size() == maxParameters());

        // Do dynamic dispatch if the type isn't fully specified
        bool dynamic = false;
        for (likely_const_expr arg : args)
            dynamic |= (arg->type == likely_matrix_multi_dimension);

        if (dynamic) {
            likely_vtable vtable = new likely_virtual_table(builder.env, body, parameters);
            builder.env->module->exprs.push_back(vtable);

            PointerType *vTableType = PointerType::getUnqual(StructType::create(builder.getContext(), "VTable"));
            Function *likelyDynamic = builder.module()->getFunction("likely_dynamic");
            if (!likelyDynamic) {
                Type *params[] = { vTableType, PointerType::get(builder.multiDimension(), 0) };
                FunctionType *likelyDynamicType = FunctionType::get(builder.multiDimension(), params, false);
                likelyDynamic = Function::Create(likelyDynamicType, GlobalValue::ExternalLinkage, "likely_dynamic", builder.module());
                likelyDynamic->setCallingConv(CallingConv::C);
                likelyDynamic->setDoesNotAlias(0);
                likelyDynamic->setDoesNotAlias(1);
                likelyDynamic->setDoesNotCapture(1);
                likelyDynamic->setDoesNotAlias(2);
                likelyDynamic->setDoesNotCapture(2);
                sys::DynamicLibrary::AddSymbol("likely_dynamic", (void*) likely_dynamic);
            }

            Value *matricies = builder.CreateAlloca(builder.multiDimension(), builder.constant(args.size()));
            for (size_t i=0; i<args.size(); i++)
                builder.CreateStore(*args[i], builder.CreateGEP(matricies, builder.constant(i)));
            Value* args[] = { ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(vtable)), uintptr_t(vtable)), vTableType), matricies };
            return new likely_expression(builder.CreateCall(likelyDynamic, args), likely_matrix_multi_dimension);
        }

        if (parameters) {
            if (parameters->type == likely_ast_list) {
                for (size_t i=0; i<args.size(); i++)
                    builder.define(parameters->atoms[i]->atom, args[i]);
            } else {
                builder.define(parameters->atom, args[0]);
            }
        }
        likely_const_expr result = builder.expression(body);
        builder.undefineAll(parameters, false);
        return result;
    }
};

class lambdaExpression : public LikelyOperator
{
    const char *symbol() const { return "->"; }
    size_t maxParameters() const { return 2; }
    likely_const_expr evaluateOperator(Builder &, likely_const_ast ast) const { return new Lambda(ast->atoms[2], ast->atoms[1]); }
};
LIKELY_REGISTER(lambda)

class beginExpression : public LikelyOperator
{
    const char *symbol() const { return "{"; }
    size_t minParameters() const { return 1; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_expr result = NULL;
        likely_env root = builder.env;
        for (size_t i=1; i<ast->num_atoms-1; i++) {
            const unique_ptr<const likely_expression> expr(builder.expression(ast->atoms[i]));
            if (!expr.get())
                goto cleanup;
        }
        result = builder.expression(ast->atoms[ast->num_atoms-1]);

    cleanup:
        while (builder.env != root) {
            likely_const_env old = builder.env;
            builder.env = const_cast<likely_env>(builder.env->parent);
            likely_release_env(old);
        }
        return result;
    }
};
LIKELY_REGISTER(begin)

struct Label : public likely_expression
{
    Label(BasicBlock *basicBlock) : likely_expression(basicBlock) {}

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
        builder.define(name.c_str(), new Label(label));
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

            const likely_matrix_type resolved = likely_type_from_types(*t, *f);

            builder.SetInsertPoint(True);
            likely_expression tc = builder.cast(*t.get(), resolved);
            builder.CreateBr(End);

            builder.SetInsertPoint(False);
            likely_expression fc = builder.cast(*f.get(), resolved);
            builder.CreateBr(End);

            builder.SetInsertPoint(End);
            PHINode *phi = builder.CreatePHI(tc.value->getType(), 2);
            phi->addIncoming(tc, True);
            if (hasElse) phi->addIncoming(fc, False);
            return new likely_expression(phi, resolved);
        } else {
            if (True->empty() || !True->back().isTerminator())
                builder.CreateBr(End);
            builder.SetInsertPoint(End);
            return new likely_expression(NULL, likely_matrix_void);
        }
    }
};
LIKELY_REGISTER(if)

struct Loop : public likely_expression
{
    string name;
    Value *start, *stop;
    BasicBlock *loop, *exit;
    BranchInst *latch;

    Loop(Builder &builder, const string &name, Value *start, Value *stop)
        : name(name), start(start), stop(stop), exit(NULL), latch(NULL)
    {
        // Loops assume at least one iteration
        BasicBlock *entry = builder.GetInsertBlock();
        loop = BasicBlock::Create(builder.getContext(), "loop_" + name, entry->getParent());
        builder.CreateBr(loop);
        builder.SetInsertPoint(loop);
        value = builder.CreatePHI(builder.nativeInt(), 2, name);
        cast<PHINode>(value)->addIncoming(start, entry);
        type = likely_matrix_native;
    }

    virtual void close(Builder &builder)
    {
        Value *increment = builder.CreateAdd(value, builder.one(), name + "_increment");
        exit = BasicBlock::Create(builder.getContext(), name + "_exit", loop->getParent());
        latch = builder.CreateCondBr(builder.CreateICmpEQ(increment, stop, name + "_test"), exit, loop);
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
        Loop loop(builder, ast->atoms[2]->atom, builder.zero(), *end);
        builder.define(ast->atoms[2]->atom, &loop);
        likely_const_expr expression = builder.expression(ast->atoms[1]);
        builder.undefine(ast->atoms[2]->atom);
        loop.close(builder);
        return expression;
    }
};
LIKELY_REGISTER(loop)

struct Assignable : public LikelyOperator
{
    Assignable(Value *value, likely_matrix_type type)
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
    class kernelArgument : public Assignable
    {
        MDNode *node;

    public:
        kernelArgument(const likely_expression &matrix, MDNode *node)
            : Assignable(matrix.value, matrix.type), node(node) {}

    private:
        Value *gep(Builder &builder, likely_const_ast ast) const
        {
            (void) ast;
            Value *columnStep, *rowStep, *frameStep;
            builder.steps(this, builder.constant(1), &columnStep, &rowStep, &frameStep);
            Value *i = builder.zero();
            if (type & likely_matrix_multi_channel) i = *builder.lookup("c");
            if (type & likely_matrix_multi_column ) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("x"), columnStep), i);
            if (type & likely_matrix_multi_row    ) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("y"), rowStep   ), i);
            if (type & likely_matrix_multi_frame  ) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("t"), frameStep ), i);
            return builder.CreateGEP(builder.data(this), i);
        }

        void set(Builder &builder, likely_const_expr expr, likely_const_ast ast) const
        {
            builder.CreateStore(builder.cast(*expr, type), gep(builder, ast));
        }

        likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
        {
            if (ast->type == likely_ast_list)
                return error(ast, "kernel operator does not take arguments");

            if (!isa<PointerType>(value->getType()))
                return new likely_expression(*this);

            LoadInst *load = builder.CreateLoad(gep(builder, ast));
            load->setMetadata("llvm.mem.parallel_loop_access", node);
            return new likely_expression(load, type & likely_matrix_element);
        }
    };

    struct kernelAxis : public Loop
    {
        kernelAxis *parent, *child;
        MDNode *node;
        Value *offset;

        kernelAxis(Builder &builder, const string &name, Value *start, Value *stop, Value *step, kernelAxis *parent)
            : Loop(builder, name, start, stop), parent(parent), child(NULL)
        {
            { // Create self-referencing loop node
                vector<Value*> metadata;
                MDNode *tmp = MDNode::getTemporary(builder.getContext(), metadata);
                metadata.push_back(tmp);
                node = MDNode::get(builder.getContext(), metadata);
                tmp->replaceAllUsesWith(node);
                MDNode::deleteTemporary(tmp);
            }

            if (parent)
                parent->child = this;
            offset = builder.CreateAdd(parent ? parent->offset : builder.zero().value, builder.CreateMul(step, value), name + "_offset");
        }

        void close(Builder &builder)
        {
            Loop::close(builder);
            latch->setMetadata("llvm.loop", node);
            if (parent) parent->close(builder);
        }

        bool referenced() const { return value->getNumUses() > 2; }

        set<string> tryCollapse(Builder &builder)
        {
            if (parent)
                return parent->tryCollapse(builder);

            set<string> collapsedAxis;
            collapsedAxis.insert(name);
            if (referenced())
                return collapsedAxis;

            while (child && !child->referenced()) {
                // Collapse the child loop into us
                child->offset->replaceAllUsesWith(value);
                child->latch->setCondition(ConstantInt::getTrue(builder.getContext()));
                DeleteDeadPHIs(child->loop);
                MergeBlockIntoPredecessor(child->loop);
                MergeBlockIntoPredecessor(child->exit);
                collapsedAxis.insert(child->name);
                child = child->child;
            }
            return collapsedAxis;
        }
    };

    struct Metadata
    {
        set<string> collapsedAxis;
    };

    const char *symbol() const { return "=>"; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        vector<likely_const_expr> srcs;
        const likely_const_ast args = ast->atoms[1];
        if (args->type == likely_ast_list) {
            for (size_t j=0; j<args->num_atoms; j++)
                srcs.push_back(builder.expression(args->atoms[j]));
        } else {
            srcs.push_back(builder.expression(args));
        }

        BasicBlock *entry = builder.GetInsertBlock();
        likely_expression dstChannels = builder.channels(srcs[0]);
        likely_expression dstColumns  = builder.columns(srcs[0]);
        likely_expression dstRows     = builder.rows(srcs[0]);
        likely_expression dstFrames   = builder.frames(srcs[0]);

        // Allocate and initialize memory for the destination matrix
        BasicBlock *allocation = BasicBlock::Create(builder.getContext(), "allocation", builder.GetInsertBlock()->getParent());
        builder.CreateBr(allocation);
        builder.SetInsertPoint(allocation);
        PHINode *kernelChannels = builder.CreatePHI(builder.nativeInt(), 1);
        PHINode *kernelColumns  = builder.CreatePHI(builder.nativeInt(), 1);
        PHINode *kernelRows     = builder.CreatePHI(builder.nativeInt(), 1);
        PHINode *kernelFrames   = builder.CreatePHI(builder.nativeInt(), 1);
        Value *kernelSize = builder.CreateMul(builder.CreateMul(builder.CreateMul(kernelChannels, kernelColumns), kernelRows), kernelFrames);

        // Finally, do the computation
        BasicBlock *computation = BasicBlock::Create(builder.getContext(), "computation", builder.GetInsertBlock()->getParent());
        builder.CreateBr(computation);
        builder.SetInsertPoint(computation);

        Metadata metadata;
        if      (builder.env->type & likely_environment_heterogeneous) metadata = generateHeterogeneous(builder, ast, srcs, kernelSize);
        else if (builder.env->type & likely_environment_parallel)      metadata = generateParallel     (builder, ast, srcs, kernelSize);
        else                                                           metadata = generateSerial       (builder, ast, srcs, kernelSize);

        kernelChannels->addIncoming(metadata.collapsedAxis.find("c") != metadata.collapsedAxis.end() ? dstChannels : builder.one(), entry);
        kernelColumns->addIncoming (metadata.collapsedAxis.find("x") != metadata.collapsedAxis.end() ? dstColumns  : builder.one(), entry);
        kernelRows->addIncoming    (metadata.collapsedAxis.find("y") != metadata.collapsedAxis.end() ? dstRows     : builder.one(), entry);
        kernelFrames->addIncoming  (metadata.collapsedAxis.find("t") != metadata.collapsedAxis.end() ? dstFrames   : builder.one(), entry);

        for (size_t i=1; i<srcs.size(); i++)
            delete srcs[i];
        return srcs[0];
    }

    Metadata generateSerial(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, Value *kernelSize) const
    {
        return generateCommon(builder, ast, srcs, builder.zero(), kernelSize);
    }

    Metadata generateParallel(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, Value *kernelSize) const
    {
        BasicBlock *entry = builder.GetInsertBlock();

        vector<Type*> parameterTypes;
        for (const likely_const_expr src : srcs)
            parameterTypes.push_back(src->value->getType());
        StructType *parameterStructType = StructType::get(builder.getContext(), parameterTypes);

        Function *thunk;
        Metadata metadata;
        {
            Type *params[] = { PointerType::getUnqual(parameterStructType), builder.nativeInt(), builder.nativeInt() };
            FunctionType *thunkType = FunctionType::get(Type::getVoidTy(builder.getContext()), params, false);

            thunk = ::cast<Function>(builder.module()->getOrInsertFunction(builder.GetInsertBlock()->getParent()->getName().str() + "_thunk", thunkType));
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
                thunkSrcs.push_back(new likely_expression(builder.CreateLoad(builder.CreateStructGEP(parameterStruct, unsigned(i))), srcs[i]->type));

            metadata = generateCommon(builder, ast, thunkSrcs, start, stop);
            const_cast<likely_expr>(srcs[0])->type = thunkSrcs[0]->type;
            for (likely_const_expr thunkSrc : thunkSrcs)
                delete thunkSrc;
            builder.CreateRetVoid();
        }

        builder.SetInsertPoint(entry);

        Type *params[] = { thunk->getType(), PointerType::getUnqual(parameterStructType), builder.nativeInt() };
        FunctionType *likelyForkType = FunctionType::get(Type::getVoidTy(builder.getContext()), params, false);
        Function *likelyFork = builder.module()->getFunction("likely_fork");
        if (!likelyFork) {
            likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "likely_fork", builder.module());
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

        builder.CreateCall3(likelyFork, builder.module()->getFunction(thunk->getName()), parameterStruct, kernelSize);
        return metadata;
    }

    Metadata generateHeterogeneous(Builder &, likely_const_ast, const vector<likely_const_expr> &, Value *) const
    {
        assert(!"Not implemented");
        return Metadata();
    }

    Metadata generateCommon(Builder &builder, likely_const_ast ast, const vector<likely_const_expr> &srcs, Value *start, Value *stop) const
    {
        Metadata metadata;
        BasicBlock *entry = builder.GetInsertBlock();
        BasicBlock *steps = BasicBlock::Create(builder.getContext(), "steps", entry->getParent());
        builder.CreateBr(steps);
        builder.SetInsertPoint(steps);
        Value *columnStep, *rowStep, *frameStep;
        builder.steps(srcs[0], builder.constant(1), &columnStep, &rowStep, &frameStep);

        kernelAxis *axis = NULL;
        for (int axis_index=0; axis_index<4; axis_index++) {
            string name;
            bool multiElement;
            Value *elements, *step;

            switch (axis_index) {
              case 0:
                name = "t";
                multiElement = (srcs[0]->type & likely_matrix_multi_frame) != 0;
                elements = builder.frames(srcs[0]);
                step = frameStep;
                break;
              case 1:
                name = "y";
                multiElement = (srcs[0]->type & likely_matrix_multi_row) != 0;
                elements = builder.rows(srcs[0]);
                step = rowStep;
                break;
              case 2:
                name = "x";
                multiElement = (srcs[0]->type & likely_matrix_multi_column) != 0;
                elements = builder.columns(srcs[0]);
                step = columnStep;
                break;
              default:
                name = "c";
                multiElement = (srcs[0]->type & likely_matrix_multi_channel) != 0;
                elements = builder.channels(srcs[0]);
                step = builder.constant(1);
                break;
            }

            if (multiElement || ((axis_index == 3) && !axis)) {
                if (!axis) axis = new kernelAxis(builder, name, start, stop, step, NULL);
                else       axis = new kernelAxis(builder, name, builder.zero(), elements, step, axis);
                builder.define(name.c_str(), axis); // takes ownership of axis
            } else {
                builder.define(name.c_str(), new likely_expression(builder.zero(), likely_matrix_native));
            }
        }
        builder.define("i", new likely_expression(axis->offset, likely_matrix_native));

        const likely_const_ast args = ast->atoms[1];
        if (args->type == likely_ast_list) {
            for (size_t j=0; j<args->num_atoms; j++)
                builder.define(args->atoms[j]->atom, new kernelArgument(*srcs[j], axis->node));
        } else {
            builder.define(args->atom, new kernelArgument(*srcs[0], axis->node));
        }

        unique_ptr<const likely_expression> result(builder.expression(ast->atoms[2]));

        const_cast<likely_expr>(srcs[0])->type = result->type;
        StoreInst *store = builder.CreateStore(builder.cast(*result, *srcs[0]), builder.CreateGEP(builder.data(srcs[0]), axis->offset));
        store->setMetadata("llvm.mem.parallel_loop_access", axis->node);

        axis->close(builder);
        metadata.collapsedAxis = axis->tryCollapse(builder);

        builder.undefineAll(args, true);
        delete builder.undefine("i");
        delete builder.undefine("c");
        delete builder.undefine("x");
        delete builder.undefine("y");
        delete builder.undefine("t");
        return metadata;
    }
};
LIKELY_REGISTER(kernel)

struct EvaluatedExpression : public LikelyOperator
{
    likely_env env;
    likely_const_ast ast;

    // Requries that `parent` stays valid through the lifetime of this class.
    // We avoid retaining `parent` to avoid a circular dependency.
    EvaluatedExpression(likely_const_env parent, likely_const_ast ast)
        : env(newEnv(parent)), ast(likely_retain_ast(ast))
    {
        likely_release_env(env->parent);
        env->type |= likely_environment_abandoned;
        env->type &= ~likely_environment_offline;
        futureResult = async(launch::deferred, [=] { return likely_eval(const_cast<likely_ast>(ast), env); });
        get(); // TODO: remove when ready to test async
    }

    ~EvaluatedExpression()
    {
        likely_release_env(get());
    }

    static likely_const_env get(likely_const_expr expr)
    {
        if (!expr || (expr->uid() != UID()))
            return NULL;
        return reinterpret_cast<const EvaluatedExpression*>(expr)->get();
    }

private:
    mutable likely_env result = NULL; // Don't access directly, call get() instead
    mutable future<likely_env> futureResult;
    mutable mutex lock;

    static int UID() { return __LINE__; }
    int uid() const { return UID(); }

    likely_env get() const
    {
        lock_guard<mutex> guard(lock);
        if (futureResult.valid()) {
            result = futureResult.get();
            likely_release_env(env);
            likely_release_ast(ast);
            const_cast<likely_env&>(env) = NULL;
            const_cast<likely_ast&>(ast) = NULL;
        }
        return result;
    }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        // TODO: implement indexing into this matrix by checking ast.
        // Consider sharing implementation with kernelArgument.
        (void) builder;
        (void) ast;

        likely_const_mat m = likely_result(get());
        if (!(m->type & likely_matrix_multi_dimension)) {
            // Promote to scalar
            return new likely_expression(builder.constant(likely_element(m, 0, 0, 0, 0), m->type & likely_matrix_element));
        } else {
            // Return the matrix
            return new likely_expression(ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(m)), uintptr_t(m)), builder.toLLVM(m->type)), m->type, likely_retain_mat(m));
        }
    }

    size_t minParameters() const { return 0; }
    size_t maxParameters() const { return 4; }
};

struct Variable : public Assignable
{
    Variable(Builder &builder, likely_const_expr expr, const string &name)
        : Assignable(builder.CreateAlloca(expr->value->getType(), 0, name), *expr)
    {
        set(builder, expr, NULL);
    }

private:
    Value *gep(Builder &builder, likely_const_ast ast) const
    {
        Value *c = (ast->num_atoms >= 2) ? builder.cast(*unique_ptr<const likely_expression>(builder.expression(ast->atoms[1])).get(), likely_matrix_native)
                                         : builder.zero();
        Value *x = (ast->num_atoms >= 3) ? builder.cast(*unique_ptr<const likely_expression>(builder.expression(ast->atoms[2])).get(), likely_matrix_native)
                                         : builder.zero();
        Value *y = (ast->num_atoms >= 4) ? builder.cast(*unique_ptr<const likely_expression>(builder.expression(ast->atoms[3])).get(), likely_matrix_native)
                                         : builder.zero();
        Value *t = (ast->num_atoms >= 5) ? builder.cast(*unique_ptr<const likely_expression>(builder.expression(ast->atoms[4])).get(), likely_matrix_native)
                                         : builder.zero();

        Value *channelStep = builder.one();
        Value *columnStep, *rowStep, *frameStep;
        builder.steps(this, channelStep, &columnStep, &rowStep, &frameStep);
        Value *i = builder.zero();
        i = builder.CreateMul(c, channelStep);
        i = builder.CreateAdd(builder.CreateMul(x, columnStep), i);
        i = builder.CreateAdd(builder.CreateMul(y, rowStep   ), i);
        i = builder.CreateAdd(builder.CreateMul(t, frameStep ), i);
        return builder.CreateGEP(builder.data(builder.CreateLoad(value), type), i);
    }

    void set(Builder &builder, likely_const_expr expr, likely_const_ast ast) const
    {
        if (!ast || (ast->type != likely_ast_list)) {
            builder.CreateStore((type & likely_matrix_multi_dimension) ? expr->value
                                                                       : builder.cast(*expr, type).value, value);
            return;
        }

        builder.CreateStore(builder.cast(*expr, type), gep(builder, ast));
    }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        if ((ast->type != likely_ast_list) || !isMat(cast<AllocaInst>(value)->getAllocatedType()))
            return new likely_expression(builder.CreateLoad(value), type);
        return new likely_expression(builder.CreateLoad(gep(builder, ast)), type & likely_matrix_element);
    }
};

class defineExpression : public LikelyOperator
{
    const char *symbol() const { return "="; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast lhs = ast->atoms[1];
        likely_const_ast rhs = ast->atoms[2];
        const char *name = likely_symbol(ast);
        likely_env env = builder.env;

        if (env->type & likely_environment_global) {
            assert(!env->value);
            if (lhs->type == likely_ast_list) {
                // Export symbol
                vector<likely_matrix_type> parameters;
                for (size_t i=1; i<lhs->num_atoms; i++) {
                    if (lhs->atoms[i]->type == likely_ast_list)
                        return error(lhs->atoms[i], "expected an atom name parameter type");
                    parameters.push_back(likely_type_from_string(lhs->atoms[i]->atom, NULL));
                }

                if (env->type & likely_environment_offline) {
                    TRY_EXPR(builder, rhs, expr);
                    const Lambda *lambda = static_cast<const Lambda*>(expr.get());
                    if (likely_const_expr function = lambda->generate(builder, parameters, name, false, false)) {
                        env->value = new Symbol(function);
                        delete function;
                    }
                } else {
                    JITFunction *function = new JITFunction(name, unique_ptr<Lambda>(new Lambda(rhs->atoms[2], rhs->atoms[1])).get(), env, parameters, true, false, false);
                    if (function->function) {
                        sys::DynamicLibrary::AddSymbol(name, function->function);
                        env->value = function;
                    } else {
                        delete function;
                    }
                }
            } else {
                if (!strcmp(likely_symbol(rhs), "->")) {
                    // Global variable
                    env->value = builder.expression(rhs);
                } else {
                    // Global value
                    env->value = new EvaluatedExpression(env, rhs);
                }
            }

            if (!env->value)
                env->type |= likely_environment_erratum;
            return NULL;
        } else {
            likely_const_expr expr = builder.expression(rhs);
            builder.define(name, expr);
            return new likely_expression(*expr);
        }
    }
};
LIKELY_REGISTER(define)

class setExpression : public LikelyOperator
{
    const char *symbol() const { return "<-"; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast lhs = ast->atoms[1];
        likely_const_ast rhs = ast->atoms[2];
        const char *name = likely_symbol(ast);
        assert(!(builder.env->type & likely_environment_global));
        likely_const_expr expr = builder.expression(rhs);
        if (expr) {
            const Assignable *assignable = Assignable::dynamicCast(builder.lookup(name));
            if (assignable) assignable->set(builder, expr, lhs);
            else            builder.define(name, new Variable(builder, expr, name));
        }
        return expr;
    }
};
LIKELY_REGISTER(set)

JITFunction::JITFunction(const string &name, const Lambda *lambda, likely_const_env parent, const vector<likely_matrix_type> &parameters, bool abandon, bool interpreter, bool arrayCC)
    : env(newEnv(parent))
{
    if (abandon) {
        likely_release_env(env->parent);
        env->type |= likely_environment_abandoned;
    }

    env->module = new likely_module();
    env->type |= likely_environment_base;
    Builder builder(env);
    associateFunction(unique_ptr<const likely_expression>(lambda->generate(builder, parameters, name, arrayCC, interpreter)).get());
    if (!value /* error */ || (interpreter && getData()) /* constant */)
        return;

// No libffi support for Windows
#ifdef _WIN32
    interpreter = false;
#endif // _WIN32

    // Don't run the interpreter on a module with loops, better to compile and execute it instead.
    if (interpreter) {
        PassManager PM;
        HasLoop *hasLoop = new HasLoop();
        PM.add(hasLoop);
        PM.run(*env->module->module);
        interpreter = !hasLoop->hasLoop;
    }

    TargetMachine *targetMachine = LikelyContext::getTargetMachine(true);
    env->module->module->setDataLayout(targetMachine->getSubtargetImpl()->getDataLayout());

    string error;
    EngineBuilder engineBuilder(unique_ptr<Module>(env->module->module));
    engineBuilder.setErrorStr(&error);

    if (interpreter) {
        engineBuilder.setEngineKind(EngineKind::Interpreter);
    } else {
        engineBuilder.setEngineKind(EngineKind::JIT)
                     .setUseMCJIT(true);
    }

    EE = engineBuilder.create(targetMachine);
    likely_assert(EE != NULL, "failed to create execution engine with error: %s", error.c_str());

    if (!interpreter) {
        EE->setObjectCache(&TheJITFunctionCache);
        if (!TheJITFunctionCache.alert(env->module->module))
            env->module->optimize();

        EE->finalizeObject();
        function = (void*) EE->getFunctionAddress(name);

        // cleanup
        EE->removeModule(env->module->module);
        env->module->finalize();
    }
//    env->module->module->dump();
}

class newExpression : public LikelyOperator
{
    const char *symbol() const { return "new"; }
    size_t maxParameters() const { return 6; }
    size_t minParameters() const { return 0; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const size_t n = ast->num_atoms - 1;
        Value *type = NULL, *channels = NULL, *columns = NULL, *rows = NULL, *frames = NULL, *data = NULL;
        switch (n) {
            case 6: data     = unique_ptr<const likely_expression>(builder.expression(ast->atoms[6]))->value;
            case 5: frames   = builder.cast(*unique_ptr<const likely_expression>(builder.expression(ast->atoms[5])).get(), likely_matrix_u32);
            case 4: rows     = builder.cast(*unique_ptr<const likely_expression>(builder.expression(ast->atoms[4])).get(), likely_matrix_u32);
            case 3: columns  = builder.cast(*unique_ptr<const likely_expression>(builder.expression(ast->atoms[3])).get(), likely_matrix_u32);
            case 2: channels = builder.cast(*unique_ptr<const likely_expression>(builder.expression(ast->atoms[2])).get(), likely_matrix_u32);
            case 1: type     = builder.cast(*unique_ptr<const likely_expression>(builder.expression(ast->atoms[1])).get(), likely_matrix_u32);
            default:           break;
        }

        switch (maxParameters()-n) {
            case 6: type     = MatrixType(builder, likely_matrix_f32);
            case 5: channels = builder.one(likely_matrix_u32);
            case 4: columns  = builder.one(likely_matrix_u32);
            case 3: rows     = builder.one(likely_matrix_u32);
            case 2: frames   = builder.one(likely_matrix_u32);
            case 1: data     = builder.nullData();
            default:           break;
        }

        likely_matrix_type inferredType = uint32_t(cast<ConstantInt>(type)->getZExtValue()) | likely_matrix_multi_dimension;
        checkDimension(inferredType, channels, likely_matrix_multi_channel);
        checkDimension(inferredType, columns , likely_matrix_multi_column);
        checkDimension(inferredType, rows    , likely_matrix_multi_row);
        checkDimension(inferredType, frames  , likely_matrix_multi_frame);

        // TODO: stack allocate scalars
        if (!(inferredType & likely_matrix_multi_dimension))
            inferredType |= likely_matrix_multi_dimension;

        return new likely_expression(builder.CreatePointerCast(builder.newMat(type, channels, columns, rows, frames, data), builder.toLLVM(inferredType)), inferredType);
    }

    static void checkDimension(likely_matrix_type &type, Value *dimension, likely_matrix_type mask)
    {
        if (ConstantInt *dim = dyn_cast<ConstantInt>(dimension))
            if (dim->getZExtValue() == 1)
                type &= ~mask;
    }
};
LIKELY_REGISTER(new)

class readExpression : public SimpleUnaryOperator
{
    const char *symbol() const { return "read"; }
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        if (likely_const_mat fileName = arg->getData())
            return builder.mat(likely_read(fileName->data, likely_file_binary));

        Function *likelyRead = builder.module()->getFunction("likely_read");
        if (!likelyRead) {
            Type *params[] = { Type::getInt8PtrTy(builder.getContext()), Type::getInt32Ty(builder.getContext()) };
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), params, false);
            likelyRead = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_read", builder.module());
            likelyRead->setCallingConv(CallingConv::C);
            likelyRead->setDoesNotAlias(0);
            likelyRead->setDoesNotAlias(1);
            likelyRead->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("likely_read", (void*) likely_read);
        }
        return new likely_expression(builder.CreateCall2(likelyRead, *arg, builder.constant(uint64_t(likely_file_binary), likely_matrix_u32)), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(read)

class writeExpression : public SimpleBinaryOperator
{
    const char *symbol() const { return "write"; }
    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &arg1, const unique_ptr<const likely_expression> &arg2) const
    {
        if (likely_const_mat image = arg1->getData())
            if (likely_const_mat fileName = arg2->getData())
                return builder.mat(likely_retain_mat(likely_write(image, fileName->data)));

        Function *likelyWrite = builder.module()->getFunction("likely_write");
        if (!likelyWrite) {
            Type *params[] = { builder.multiDimension(), Type::getInt8PtrTy(builder.getContext()) };
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), params, false);
            likelyWrite = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_write", builder.module());
            likelyWrite->setCallingConv(CallingConv::C);
            likelyWrite->setDoesNotAlias(0);
            likelyWrite->setDoesNotAlias(1);
            likelyWrite->setDoesNotCapture(1);
            likelyWrite->setDoesNotAlias(2);
            likelyWrite->setDoesNotCapture(2);
            sys::DynamicLibrary::AddSymbol("likely_write", (void*) likely_write);
        }
        return new likely_expression(builder.retainMat(builder.CreateCall2(likelyWrite, *arg1, *arg2)), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(write)

class decodeExpression : public SimpleUnaryOperator
{
    const char *symbol() const { return "decode"; }
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        if (likely_const_mat buffer = arg->getData())
            return builder.mat(likely_decode(buffer));

        Function *likelyDecode = builder.module()->getFunction("likely_decode");
        if (!likelyDecode) {
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), builder.multiDimension(), false);
            likelyDecode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_decode", builder.module());
            likelyDecode->setCallingConv(CallingConv::C);
            likelyDecode->setDoesNotAlias(0);
            likelyDecode->setDoesNotAlias(1);
            likelyDecode->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("likely_decode", (void*) likely_decode);
        }
        return new likely_expression(builder.CreateCall(likelyDecode, builder.CreatePointerCast(*arg, builder.multiDimension())), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(decode)

class encodeExpression : public SimpleBinaryOperator
{
    const char *symbol() const { return "encode"; }
    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &arg1, const unique_ptr<const likely_expression> &arg2) const
    {
        if (likely_const_mat image = arg1->getData())
            if (likely_const_mat extension = arg2->getData())
                return builder.mat(likely_encode(image, extension->data));

        Function *likelyEncode = builder.module()->getFunction("likely_encode");
        if (!likelyEncode) {
            Type *params[] = { builder.multiDimension(), Type::getInt8PtrTy(builder.getContext()) };
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), params, false);
            likelyEncode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_encode", builder.module());
            likelyEncode->setCallingConv(CallingConv::C);
            likelyEncode->setDoesNotAlias(0);
            likelyEncode->setDoesNotAlias(1);
            likelyEncode->setDoesNotCapture(1);
            likelyEncode->setDoesNotAlias(2);
            likelyEncode->setDoesNotCapture(2);
            sys::DynamicLibrary::AddSymbol("likely_encode", (void*) likely_encode);
        }
        return new likely_expression(builder.CreateCall2(likelyEncode, builder.CreatePointerCast(*arg1, builder.multiDimension()), *arg2), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(encode)

class md5Expression : public SimpleUnaryOperator
{
    const char *symbol() const { return "md5"; }
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        Function *likelyMd5 = builder.module()->getFunction("likely_md5");
        if (!likelyMd5) {
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), builder.multiDimension(), false);
            likelyMd5 = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_md5", builder.module());
            likelyMd5->setCallingConv(CallingConv::C);
            likelyMd5->setDoesNotAlias(0);
            likelyMd5->setDoesNotAlias(1);
            likelyMd5->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("likely_md5", (void*) likely_md5);
        }
        return new likely_expression(builder.CreateCall(likelyMd5, *arg), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(md5)

} // namespace (anonymous)

likely_env likely_jit()
{
    return newEnv(RootEnvironment::get());
}

likely_env likely_static(const char *file_name)
{
    likely_env env = newEnv(RootEnvironment::get());
    if (!env)
        return NULL;

    env->module = new OfflineModule(file_name);
    env->type |= likely_environment_offline;
    env->type |= likely_environment_base;
    return env;
}

likely_env likely_retain_env(likely_const_env env)
{
    if (!env) return NULL;
    assert(env->ref_count > 0);
    const_cast<likely_env>(env)->ref_count++;
    return const_cast<likely_env>(env);
}

void likely_release_env(likely_const_env env)
{
    if (!env) return;
    assert(env->ref_count > 0);
    if (--const_cast<likely_env>(env)->ref_count) return;

    delete env->value; // Do this early to guarantee the environment for these lifetime of these classes
    likely_release_ast(env->ast);
    if (env->type & likely_environment_base)
        delete env->module;
    if (!(env->type & likely_environment_abandoned))
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
        vector<likely_matrix_type> types;
        for (size_t i=0; i<vtable->n; i++)
            types.push_back(mats[i]->type);
        vtable->functions.push_back(unique_ptr<JITFunction>(new JITFunction("likely_vtable_entry", unique_ptr<Lambda>(new Lambda(vtable->body, vtable->parameters)).get(), vtable->env, types, true, false, true)));
        function = vtable->functions.back()->function;
        if (function == NULL)
            return NULL;
    }

    return reinterpret_cast<likely_mat (*)(likely_const_mat const*)>(function)(mats);
}

void *likely_compile(likely_const_env env, likely_matrix_type const *type, uint32_t n)
{
    return Lambda::getFunction(env, vector<likely_matrix_type>(type, type + n));
}

likely_const_mat likely_result(likely_const_env env)
{
    if (!env)
        return NULL;
    if (likely_const_env rhs = EvaluatedExpression::get(env->value))
        return likely_result(rhs);
    return Lambda::getResult(env);
}

likely_env likely_eval(likely_ast ast, likely_env parent)
{
    if (!ast)
        return NULL;

    likely_env env = newEnv(parent);
    if ((ast->type == likely_ast_list) && (ast->num_atoms > 0) && !strcmp(ast->atoms[0]->atom, "="))
        env->type |= likely_environment_definition;
    env->type |= likely_environment_global;
    env->ast = likely_retain_ast(ast);

    if (env->type & likely_environment_definition) {
        likely_const_expr expr = Builder(env).expression(ast);
        assert(!expr); (void) expr;
    } else if (env->type & likely_environment_offline) {
        // Do nothing, evaluating expressions in an offline environment is a no-op.
    } else {
        // If `ast` is not a lambda then it is a computation and we want to represent it as a parameterless lambda.
        if (!strcmp(likely_symbol(ast), "->")) env->value = Builder(env).expression(ast);
        else                                   env->value = new Lambda(ast);
    }

    likely_assert(env->ref_count == 1, "returning an environment with: %d owners", env->ref_count);
    return env;
}

likely_env likely_repl(likely_ast ast, likely_env parent, likely_repl_callback repl_callback, void *context)
{
    if (!ast || (ast->type != likely_ast_list))
        return NULL;

    likely_env env = likely_retain_env(parent);
    for (size_t i=0; i<ast->num_atoms; i++) {
        if (!ast->atoms[i])
            continue;

        env = likely_eval(ast->atoms[i], parent);
        likely_release_env(parent);
        parent = env;
        if (repl_callback)
            // If there is not context, we return a boolean value indicating if the environment has a valid result
            repl_callback(env, context ? context : (void*)(!(env->type & likely_environment_definition)));
        if (env->type & likely_environment_erratum)
            break;
    }

    return env;
}

likely_mat likely_md5(likely_const_mat mat)
{
    MD5 md5;
    md5.update(ArrayRef<uint8_t>(reinterpret_cast<const uint8_t*>(mat->data), likely_bytes(mat)));
    MD5::MD5Result md5Result;
    md5.final(md5Result);
    return likely_new(likely_matrix_u8, 16, 1, 1, 1, md5Result);
}

void likely_shutdown()
{
    likely_release_env(RootEnvironment::get());
    LikelyContext::shutdown();
    llvm_shutdown();
}
