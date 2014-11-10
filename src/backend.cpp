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
#include "likely/io.h"

using namespace llvm;
using namespace std;

static int OptLevel = 0;
static int SizeLevel = 0;
static bool LoopVectorize = false;

void likely_initialize(int opt_level, int size_level, bool loop_vectorize)
{
    OptLevel = max(min(opt_level, 3), 0);
    SizeLevel = max(min(size_level, 2), 0);
    LoopVectorize = loop_vectorize;

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

    sys::DynamicLibrary::AddSymbol("likely_read", (void*) likely_read);
    sys::DynamicLibrary::AddSymbol("likely_write", (void*) likely_write);
    sys::DynamicLibrary::AddSymbol("likely_decode", (void*) likely_decode);
    sys::DynamicLibrary::AddSymbol("likely_encode", (void*) likely_encode);
}

namespace {

class LikelyContext
{
    static queue<LikelyContext*> contextPool;
    map<likely_matrix_type, Type*> typeLUT;
    PassManager *PM;

    // use LikelyContext::acquire()
    LikelyContext()
        : PM(new PassManager())
    {
        if (OptLevel > 0) {
            PM->add(createVerifierPass());
            static TargetMachine *TM = getTargetMachine(false);
            PM->add(new TargetLibraryInfo(Triple(sys::getProcessTriple())));
            PM->add(new DataLayoutPass(*TM->getSubtargetImpl()->getDataLayout()));
            TM->addAnalysisPasses(*PM);
            PassManagerBuilder builder;
            builder.OptLevel = OptLevel;
            builder.SizeLevel = SizeLevel;
            builder.LoopVectorize = LoopVectorize;
            builder.Inliner = createAlwaysInlinerPass();
            builder.populateModulePassManager(*PM);
            PM->add(createVerifierPass());
        } else {
            PM = NULL;
        }
    }

    // use LikelyContext::release()
    ~LikelyContext()
    {
        delete PM;
    }

    LikelyContext(const LikelyContext &) = delete;
    LikelyContext &operator=(const LikelyContext &) = delete;

public:
    LLVMContext context;

    static LikelyContext *acquire()
    {
        static mutex lock;
        lock_guard<mutex> guard(lock);

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
        auto result = typeLUT.find(likely);
        if (result != typeLUT.end())
            return result->second;

        Type *llvm;
        if (likely & likely_matrix_multi_dimension) {
            const likely_mat str = likely_type_to_string(likely);
            llvm = PointerType::getUnqual(StructType::create(str->data,
                                                             Type::getInt32Ty(context), // ref_count
                                                             Type::getInt32Ty(context), // type
                                                             Type::getInt32Ty(context), // channels
                                                             Type::getInt32Ty(context), // columns
                                                             Type::getInt32Ty(context), // rows
                                                             Type::getInt32Ty(context), // frames
                                                             ArrayType::get(Type::getInt8Ty(context), 0), // data
                                                             NULL));
            likely_release_mat(str);

        } else if (likely == likely_matrix_void) {
            llvm = Type::getVoidTy(context);
        } else {
            llvm = scalar(likely);
        }

        if (likely & likely_matrix_pointer)
            llvm = PointerType::getUnqual(llvm);

        typeLUT[likely] = llvm;
        return llvm;
    }

    void optimize(Module &module)
    {
        if (!PM)
            return;

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

    env->type = 0;
    if (parent) {
        if (parent->type & likely_environment_parallel     ) env->type |= likely_environment_parallel;
        if (parent->type & likely_environment_heterogeneous) env->type |= likely_environment_heterogeneous;
        if (parent->type & likely_environment_ctfe         ) env->type |= likely_environment_ctfe;
    }
    env->parent = likely_retain_env(parent);
    env->ast = NULL;
    env->module = parent ? parent->module : NULL;
    env->expr = NULL;
    env->ref_count = 1;
    return env;
}

struct LikelyValue
{
    Value *value;
    likely_matrix_type type;

    LikelyValue(Value *value = NULL, likely_matrix_type type = likely_matrix_void)
        : value(value), type(type)
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

    operator Value*() const { return value; }
    operator likely_matrix_type() const { return type; }

    void dump() const
    {
        likely_const_mat m = likely_type_to_string(type);
        cerr << m->data << " ";
        value->dump();
        likely_release_mat(m);
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
                            type |= likely_matrix_pointer;
                        return type;
                    }
                } else {
                    return likely_matrix_void;
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

class Variant
{
    union {
        void *value = NULL;
        likely_const_ast ast;
        likely_const_env env;
        likely_const_mat mat;
    };

public:
    enum Type
    {
        Mat,
        Ast,
        Env,
    } type;

    Variant(void *value, Type type)
        : value(value), type(type) {}

    Variant()
        : Variant(NULL, Mat) {}

    Variant(likely_const_mat mat)
        : Variant((void*) mat, Mat) {}

    ~Variant()
    {
        switch (type) {
          case Ast: likely_release_ast(ast); break;
          case Env: likely_release_env(env); break;
          case Mat: likely_release_mat(mat); break;
        }
    }

    Variant(const Variant &other)
    {
        *this = other;
    }

    Variant &operator=(const Variant &other)
    {
        switch (other.type) {
          case Ast: value = likely_retain_ast(other); break;
          case Env: value = likely_retain_env(other); break;
          case Mat: value = likely_retain_mat(other); break;
        }
        type = other.type;
        return *this;
    }

    operator bool() const { return value != NULL; }
    operator likely_const_ast() const { assert(type == Ast); return ast; }
    operator likely_const_env() const { assert(type == Env); return env; }
    operator likely_const_mat() const { assert(type == Mat); return mat; }
};

typedef struct likely_expression *likely_expr;
typedef struct likely_expression const *likely_const_expr;

struct likely_expression : public LikelyValue
{
    likely_expression(const LikelyValue &value = LikelyValue(), const Variant &data = Variant())
        : LikelyValue(value), data(data) {}

    virtual ~likely_expression() {}
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
            likely_set_element(m, (type & likely_matrix_signed) ? double(constantInt->getSExtValue())
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

    static likely_const_expr get(Builder &builder, likely_const_ast ast);

    static likely_const_expr error(likely_const_ast ast, const char *message)
    {
        likely_throw(ast, message);
        return NULL;
    }

    static void define(likely_const_env &env, const char *name, likely_const_expr value)
    {
        assert(name && strcmp(name, ""));
        likely_env child = newEnv(env);
        child->type |= likely_environment_definition;
        child->ast = likely_atom(name, uint32_t(strlen(name)));
        child->expr = value;
        env = child;
    }

    static likely_const_expr undefine(likely_const_env &env, const char *name)
    {
        assert(env->type & likely_environment_definition);
        likely_assert(!strcmp(name, likely_symbol(env->ast)), "undefine variable mismatch");
        likely_env old = const_cast<likely_env>(env);
        likely_const_expr value = NULL;
        swap(value, old->expr);
        env = old->parent;
        likely_release_env(old);
        return value;
    }

    static void undefineAll(likely_const_env &env, likely_const_ast args, bool deleteExpression)
    {
        if (args->type == likely_ast_list) {
            for (size_t i=0; i<args->num_atoms; i++) {
                likely_const_expr expression = undefine(env, args->atoms[args->num_atoms-i-1]->atom);
                if (deleteExpression) delete expression;
            }
        } else {
            likely_const_expr expression = undefine(env, args->atom);
            if (deleteExpression) delete expression;
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

    likely_module(const likely_module &) = delete;
    likely_module &operator=(const likely_module &) = delete;

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
        optimize();

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

    OfflineModule(const OfflineModule &) = delete;
    OfflineModule &operator=(const OfflineModule &) = delete;
};

struct Builder : public IRBuilder<>
{
    likely_const_env env;
    likely_module *module;

    Builder(likely_const_env env, likely_module *module)
        : IRBuilder<>(module ? module->context->context : getGlobalContext()), env(env), module(module) {}

    LikelyValue constant(uint64_t value, likely_matrix_type type = likely_matrix_native)
    {
        const unsigned depth = unsigned(type & likely_matrix_depth);
        return LikelyValue(Constant::getIntegerValue(Type::getIntNTy(getContext(), depth), APInt(depth, value)), type);
    }

    LikelyValue constant(double value, likely_matrix_type type)
    {
        const size_t depth = type & likely_matrix_depth;
        if (type & likely_matrix_floating) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return LikelyValue(ConstantFP::get(Type::getDoubleTy(getContext()), value), type);
            else if (depth == 32) return LikelyValue(ConstantFP::get(Type::getFloatTy(getContext()), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return LikelyValue(); }
        } else {
            return constant(uint64_t(value), type);
        }
    }

    LikelyValue zero(likely_matrix_type type = likely_matrix_native) { return constant(0.0, type); }
    LikelyValue one (likely_matrix_type type = likely_matrix_native) { return constant(1.0, type); }
    LikelyValue intMax(likely_matrix_type type) { const likely_matrix_type bits = type & likely_matrix_depth; return constant((uint64_t) (1 << (bits - ((type & likely_matrix_signed) ? 1 : 0)))-1, bits); }
    LikelyValue intMin(likely_matrix_type type) { const likely_matrix_type bits = type & likely_matrix_depth; return constant((uint64_t) ((type & likely_matrix_signed) ? (1 << (bits - 1)) : 0), bits); }
    LikelyValue nullMat() { return LikelyValue(ConstantPointerNull::get(::cast<PointerType>((Type*)multiDimension())), likely_matrix_multi_dimension); }
    LikelyValue nullData() { return LikelyValue(ConstantPointerNull::get(Type::getInt8PtrTy(getContext())), likely_matrix_u8 | likely_matrix_pointer); }

    Value *addInts(Value *lhs, Value *rhs)
    {
        if (Constant *c = dyn_cast<Constant>(lhs))
            if (c->isZeroValue())
                return rhs;
        if (Constant *c = dyn_cast<Constant>(rhs))
            if (c->isZeroValue())
                return lhs;
        return CreateAdd(lhs, rhs);
    }

    Value *multiplyInts(Value *lhs, Value *rhs)
    {
        if (Constant *c = dyn_cast<Constant>(lhs))
            if (c->isOneValue())
                return rhs;
        if (Constant *c = dyn_cast<Constant>(rhs))
            if (c->isOneValue())
                return lhs;
        return CreateMul(lhs, rhs);
    }

    // channels(), columns(), rows() and frames() return native integers by design
    LikelyValue channels(const LikelyValue &m) { return (m & likely_matrix_multi_channel) ? cast(LikelyValue(CreateLoad(CreateStructGEP(m, 2), "channels"), likely_matrix_u32), likely_matrix_native) : one(); }
    LikelyValue columns (const LikelyValue &m) { return (m & likely_matrix_multi_column ) ? cast(LikelyValue(CreateLoad(CreateStructGEP(m, 3), "columns" ), likely_matrix_u32), likely_matrix_native) : one(); }
    LikelyValue rows    (const LikelyValue &m) { return (m & likely_matrix_multi_row    ) ? cast(LikelyValue(CreateLoad(CreateStructGEP(m, 4), "rows"    ), likely_matrix_u32), likely_matrix_native) : one(); }
    LikelyValue frames  (const LikelyValue &m) { return (m & likely_matrix_multi_frame  ) ? cast(LikelyValue(CreateLoad(CreateStructGEP(m, 5), "frames"  ), likely_matrix_u32), likely_matrix_native) : one(); }
    LikelyValue data    (const LikelyValue &m) { return LikelyValue(CreatePointerCast(CreateStructGEP(m, 6), module->context->scalar(m, true)), (m & likely_matrix_element) | likely_matrix_pointer); }

    LikelyValue cast(const LikelyValue &x, likely_matrix_type type)
    {
        type &= likely_matrix_element;
        if ((x.type & likely_matrix_element) == type)
            return LikelyValue(x, type);
        if ((type & likely_matrix_depth) == 0) {
            type |= x.type & likely_matrix_depth;
            if (type & likely_matrix_floating)
                type = likely_type_from_types(type, likely_matrix_floating);
        }
        Type *dstType = module->context->scalar(type, type & likely_matrix_pointer);
        return LikelyValue(CreateCast(CastInst::getCastOpcode(x, (x & likely_matrix_signed) != 0, dstType, (type & likely_matrix_signed) != 0), x, dstType), type);
    }

    IntegerType *nativeInt() { return module->context->nativeInt(); }
    Type *multiDimension() { return toLLVM(likely_matrix_multi_dimension); }
    Type *toLLVM(likely_matrix_type likely) { return module->context->toLLVM(likely); }

    LikelyValue toMat(const LikelyValue &expr)
    {
        if (LikelyValue::isMat(expr.value->getType()))
            return expr;

        if (expr.value->getType()->isPointerTy() /* assume it's a string for now */) {
            Function *likelyString = module->module->getFunction("likely_string");
            if (!likelyString) {
                FunctionType *functionType = FunctionType::get(toLLVM(likely_matrix_string), Type::getInt8PtrTy(getContext()), false);
                likelyString = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_string", module->module);
                likelyString->setCallingConv(CallingConv::C);
                likelyString->setDoesNotAlias(0);
                likelyString->setDoesNotAlias(1);
                likelyString->setDoesNotCapture(1);
                sys::DynamicLibrary::AddSymbol("likely_string", (void*) likely_string);
            }
            return LikelyValue(CreateCall(likelyString, expr), likely_matrix_string);
        }

        Function *likelyScalar = module->module->getFunction("likely_scalar");
        if (!likelyScalar) {
            Type *params[] = { Type::getInt32Ty(getContext()), Type::getDoublePtrTy(getContext()), Type::getInt32Ty(getContext()) };
            FunctionType *functionType = FunctionType::get(multiDimension(), params, false);
            likelyScalar = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_scalar", module->module);
            likelyScalar->setCallingConv(CallingConv::C);
            likelyScalar->setDoesNotAlias(0);
            likelyScalar->setDoesNotAlias(2);
            likelyScalar->setDoesNotCapture(2);
            sys::DynamicLibrary::AddSymbol("likely_scalar", (void*) likely_scalar);
        }

        AllocaInst *allocaInst = CreateAlloca(Type::getDoubleTy(getContext()), one());
        CreateStore(cast(expr, likely_matrix_f64), allocaInst);
        return LikelyValue(CreateCall3(likelyScalar, constant(uint64_t(expr.type), likely_matrix_u32), allocaInst, one(likely_matrix_u32)), likely_matrix_multi_dimension);
    }

    LikelyValue newMat(Value *type, Value *channels, Value *columns, Value *rows, Value *frames, Value *data)
    {
        Function *likelyNew = module->module->getFunction("likely_new");
        if (!likelyNew) {
            Type *params[] = { Type::getInt32Ty(getContext()), Type::getInt32Ty(getContext()), Type::getInt32Ty(getContext()), Type::getInt32Ty(getContext()), Type::getInt32Ty(getContext()), Type::getInt8PtrTy(getContext()) };
            FunctionType *functionType = FunctionType::get(multiDimension(), params, false);
            likelyNew = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_new", module->module);
            likelyNew->setCallingConv(CallingConv::C);
            likelyNew->setDoesNotAlias(0);
            likelyNew->setDoesNotAlias(6);
            likelyNew->setDoesNotCapture(6);
            sys::DynamicLibrary::AddSymbol("likely_new", (void*) likely_new);
        }
        Value* args[] = { type, channels, columns, rows, frames, data };
        return LikelyValue(CreateCall(likelyNew, args), likely_matrix_multi_dimension);
    }

    LikelyValue retainMat(Value *m)
    {
        Function *likelyRetain = module->module->getFunction("likely_retain_mat");
        if (!likelyRetain) {
            FunctionType *functionType = FunctionType::get(multiDimension(), multiDimension(), false);
            likelyRetain = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_retain_mat", module->module);
            likelyRetain->setCallingConv(CallingConv::C);
            likelyRetain->setDoesNotAlias(1);
            likelyRetain->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("likely_retain_mat", (void*) likely_retain_mat);
        }
        return LikelyValue(CreateCall(likelyRetain, CreatePointerCast(m, multiDimension())), likely_matrix_multi_dimension);
    }

    LikelyValue releaseMat(Value *m)
    {
        Function *likelyRelease = module->module->getFunction("likely_release_mat");
        if (!likelyRelease) {
            FunctionType *functionType = FunctionType::get(Type::getVoidTy(getContext()), multiDimension(), false);
            likelyRelease = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_release_mat", module->module);
            likelyRelease->setCallingConv(CallingConv::C);
            likelyRelease->setDoesNotAlias(1);
            likelyRelease->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("likely_release_mat", (void*) likely_release_mat);
        }
        return LikelyValue(CreateCall(likelyRelease, CreatePointerCast(m, multiDimension())), likely_matrix_void);
    }
};

class ConstantMat : public likely_expression
{
    ConstantMat(likely_const_mat m)
        : likely_expression(LikelyValue(), m) {}

    likely_const_expr evaluate(Builder &builder, likely_const_ast) const
    {
        const likely_const_mat m = getData();
        const LikelyValue value = (m->type & likely_matrix_multi_dimension) ? LikelyValue(ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(likely_mat)), uintptr_t(m)), builder.toLLVM(m->type)), m->type)
                                                                            : LikelyValue(builder.constant(likely_element(m, 0, 0, 0, 0), m->type), m->type);

        // Make sure the lifetime of the data is at least as long as the lifetime of the code.
        if (m->type & likely_matrix_multi_dimension)
            builder.module->mats.push_back(likely_retain_mat(m));

        return new likely_expression(value, likely_retain_mat(m));
    }

public:
    static likely_const_expr get(likely_const_mat data)
    {
        if (!data)
            return NULL;
        return new ConstantMat(data);
    }

    static likely_const_expr get(Builder &builder, likely_const_mat data)
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
    const vector<likely_matrix_type> parameters;

    Symbol(const string &name, likely_matrix_type returnType, vector<likely_matrix_type> parameters = vector<likely_matrix_type>())
        : likely_expression(LikelyValue(NULL, returnType)), name(name), parameters(parameters) {}

private:
    size_t maxParameters() const { return parameters.size(); }

    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (parameters.size() != ((ast->type == likely_ast_list) ? ast->num_atoms-1 : 0))
            return error(ast, "incorrect argument count");

        Function *symbol = builder.module->module->getFunction(name);
        if (!symbol) {
            // Translate definition type across contexts
            vector<Type*> llvmParameters;
            for (likely_matrix_type parameter : parameters)
                llvmParameters.push_back(builder.toLLVM(parameter));
            Type *llvmReturn = builder.toLLVM(type);
            FunctionType *functionType = FunctionType::get(llvmReturn, llvmParameters, false);
            symbol = Function::Create(functionType, GlobalValue::ExternalLinkage, name, builder.module->module);
            symbol->setCallingConv(CallingConv::C);
            if (isa<PointerType>(llvmReturn))
                symbol->setDoesNotAlias(0);
            for (size_t i=0; i<llvmParameters.size(); i++)
                if (isa<PointerType>(llvmParameters[i])) {
                    symbol->setDoesNotAlias(i+1);
                    symbol->setDoesNotCapture(i+1);
                }
        }

        vector<Value*> args;
        if (ast->type == likely_ast_list)
            for (size_t i=1; i<ast->num_atoms; i++) {
                TRY_EXPR(builder, ast->atoms[i], arg)
                if (isa<PointerType>(arg->value->getType()))
                    args.push_back(builder.CreatePointerCast(*arg, builder.toLLVM(parameters[i-1])));
                else
                    args.push_back(builder.cast(*arg.get(), parameters[i-1]));
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
    Variant::Type returnType = Variant::Mat;

    JITFunction(const string &name, const Lambda *lambda, const vector<likely_matrix_type> &parameters, bool evaluate, bool arrayCC);

    ~JITFunction()
    {
        if (EE && module->module) // interpreter
            EE->removeModule(module->module);
        delete EE;
        delete module;
    }

    JITFunction(const JITFunction &) = delete;
    JITFunction &operator=(const JITFunction &) = delete;

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

struct likely_virtual_table : public LikelyOperator
{
    const likely_const_env env;
    const likely_const_ast body, parameters;
    const size_t n;
    vector<unique_ptr<JITFunction>> functions;

    likely_virtual_table(likely_const_env env, likely_const_ast body, likely_const_ast parameters)
        : env(likely_retain_env(env)), body(likely_retain_ast(body)), parameters(likely_retain_ast(parameters)), n(length(parameters)) {}

    ~likely_virtual_table()
    {
        likely_release_ast(parameters);
        likely_release_ast(body);
        likely_release_env(env);
    }

    likely_virtual_table(const likely_virtual_table &) = delete;
    likely_virtual_table &operator=(const likely_virtual_table &) = delete;

private:
    likely_const_expr evaluateOperator(Builder &, likely_const_ast) const { return NULL; }
};

namespace {

struct RootEnvironment
{
    // Provide public access to an environment that includes the standard library.
    static likely_const_env get()
    {
        static bool init = false;
        if (!init) {
            likely_ast ast = likely_lex_and_parse(likely_standard_library, likely_file_gfm);
            builtins() = likely_eval(ast, builtins(), NULL, NULL);
            likely_release_ast(ast);
            init = true;
        }

        return builtins();
    }

protected:
    // Provide protected access for registering builtins.
    static likely_const_env &builtins()
    {
        static likely_const_env root = NULL;
        if (!root) {
            root = newEnv(NULL);
            const_cast<likely_env>(root)->type |= likely_environment_ctfe;
        }
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
        likely_expression xc(builder.cast(*x.get(), likely_type_from_types(*x, likely_matrix_floating)));
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
        if (builder.env->type & likely_environment_ctfe) {
            if (likely_const_mat LHS = lhs.getData()) {
                if (likely_const_mat RHS = rhs.getData()) {
                    static map<const char*, likely_const_env> envLUT;
                    static map<const char*, void*> functionLUT;
                    static mutex lock;

                    lock.lock();
                    auto function = functionLUT.find(symbol());
                    if (function == functionLUT.end()) {
                        const string code = string("(a b) :-> { dst := a.imitate (dst a b) :=> (<- dst (") + symbol() + string(" a b)) }");
                        const likely_ast ast = likely_lex_and_parse(code.c_str(), likely_file_lisp);
                        const likely_env parent = likely_standard(NULL);
                        const likely_env env = likely_eval(ast, parent, NULL, NULL);
                        assert(env->expr);
                        void *f = likely_compile(env->expr, NULL, 0);
                        likely_release_env(parent);
                        likely_release_ast(ast);
                        envLUT.insert(pair<const char*, likely_const_env>(symbol(), env));
                        functionLUT.insert(pair<const char*, void*>(symbol(), f));
                        function = functionLUT.find(symbol());
                    }
                    lock.unlock();

                    return ConstantMat::get(builder, reinterpret_cast<likely_mat (*)(likely_const_mat, likely_const_mat)>(function->second)(LHS, RHS));
                }
            }
        }

        return new likely_expression(LikelyValue(evaluateSimpleArithmetic(builder, lhs, rhs), lhs));
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
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module->module, (lhs.type & likely_matrix_signed) ? Intrinsic::sadd_with_overflow : Intrinsic::uadd_with_overflow, lhs.value->getType()), lhs, rhs);
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

        const likely_matrix_type type = likely_type_from_types(*expr1, *expr2);
        const likely_expression lhs(builder.cast(*expr1.get(), type));
        const likely_expression rhs(builder.cast(*expr2.get(), type));

        if (type & likely_matrix_floating) {
            return new likely_expression(LikelyValue(builder.CreateFSub(lhs, rhs), type));
        } else {
            if (type & likely_matrix_saturated) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module->module, (lhs.type & likely_matrix_signed) ? Intrinsic::ssub_with_overflow : Intrinsic::usub_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = (lhs.type & likely_matrix_signed) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, builder.zero(lhs)), builder.intMax(lhs), builder.intMin(lhs)) : builder.intMin(lhs).value;
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
        if (lhs & likely_matrix_floating) {
            return builder.CreateFMul(lhs, rhs);
        } else {
            if (lhs.type & likely_matrix_saturated) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module->module, (lhs.type & likely_matrix_signed) ? Intrinsic::smul_with_overflow : Intrinsic::umul_with_overflow, lhs.value->getType()), lhs, rhs);
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

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                                                                         \
class OP##Expression : public ArithmeticOperator                                                                                                                                    \
{                                                                                                                                                                                   \
    const char *symbol() const { return #SYM; }                                                                                                                                     \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const                                                        \
    {                                                                                                                                                                               \
        return new likely_expression(LikelyValue((lhs.type & likely_matrix_floating) ? builder.CreateFCmpO##OP(lhs, rhs)                                                            \
                                                                                     : ((lhs.type & likely_matrix_signed) ? builder.CreateICmpS##OP(lhs, rhs)                       \
                                                                                                                          : builder.CreateICmpU##OP(lhs, rhs)), likely_matrix_u1)); \
    }                                                                                                                                                                               \
};                                                                                                                                                                                  \
LIKELY_REGISTER(OP)                                                                                                                                                                 \

LIKELY_REGISTER_COMPARISON(LT, <)
LIKELY_REGISTER_COMPARISON(LE, <=)
LIKELY_REGISTER_COMPARISON(GT, >)
LIKELY_REGISTER_COMPARISON(GE, >=)

#define LIKELY_REGISTER_EQUALITY(OP, SYM)                                                                                        \
class OP##Expression : public ArithmeticOperator                                                                                 \
{                                                                                                                                \
    const char *symbol() const { return #SYM; }                                                                                  \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const     \
    {                                                                                                                            \
        return new likely_expression(LikelyValue((lhs.type & likely_matrix_floating) ? builder.CreateFCmpO##OP(lhs, rhs)         \
                                                                         : builder.CreateICmp##OP(lhs, rhs), likely_matrix_u1)); \
    }                                                                                                                            \
};                                                                                                                               \
LIKELY_REGISTER(OP)                                                                                                              \

LIKELY_REGISTER_EQUALITY(EQ, ==)
LIKELY_REGISTER_EQUALITY(NE, !=)

class BinaryMathOperator : public SimpleBinaryOperator
{
    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &x, const unique_ptr<const likely_expression> &n) const
    {
        const likely_matrix_type type = likely_type_from_types(likely_type_from_types(*x, *n), likely_matrix_floating);
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
            if (const likely_const_mat mat = likely_result(env))
                value = ConstantMat::get(builder, likely_retain_mat(mat));
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
    likely_const_env env;
    likely_const_ast body, parameters;

    Lambda(likely_const_env env, likely_const_ast body, likely_const_ast parameters = NULL)
        : env(env), body(body), parameters(parameters) {}

    ~Lambda()
    {
        for (JITFunction *jitFunction : jitFunctions)
            delete jitFunction;
    }

    Lambda(const Lambda &) = delete;
    Lambda &operator=(const Lambda &) = delete;

    likely_const_expr generate(Builder &builder, vector<likely_matrix_type> parameters, string name, bool arrayCC, bool promoteScalarToMatrix) const
    {
        likely_const_env restore = builder.env;
        builder.env = newEnv(env);
        const_cast<likely_env>(builder.env)->type = restore->type;

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
        Function *tmpFunction = cast<Function>(builder.module->module->getOrInsertFunction(name+"_tmp", FunctionType::get(Type::getVoidTy(builder.getContext()), llvmTypes, false)));
        BasicBlock *entry = BasicBlock::Create(builder.getContext(), "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        vector<likely_const_expr> arguments;
        if (arrayCC) {
            Value *argumentArray = tmpFunction->arg_begin();
            for (size_t i=0; i<parameters.size(); i++) {
                Value *load = builder.CreateLoad(builder.CreateGEP(argumentArray, builder.constant(i)));
                if (parameters[i] & likely_matrix_multi_dimension) {
                    load = builder.CreatePointerCast(load, builder.toLLVM(parameters[i]));
                } else {
                    const likely_matrix_type tmpType = parameters[i] | likely_matrix_multi_dimension;
                    const LikelyValue tmpValue(builder.CreatePointerCast(load, builder.toLLVM(tmpType)), tmpType);
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
        for (likely_const_expr arg : arguments)
            delete arg;
        if (!result)
            return NULL;

        // If we are expecting a constant or a matrix and don't get one then make a matrix
        if (promoteScalarToMatrix && !result->getData() && !dyn_cast<PointerType>(result->value->getType()))
            result.reset(new likely_expression(builder.toMat(*result)));

        // If we are returning a constant matrix, make sure to retain a copy
        if (isa<ConstantExpr>(result->value) && isMat(result->value->getType()))
            result.reset(new likely_expression(LikelyValue(builder.CreatePointerCast(builder.retainMat(result->value), builder.toLLVM(result->type)), result->type), likely_retain_mat(result->getData())));

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

        likely_release_env(builder.env);
        builder.env = restore;
        return new likely_expression(LikelyValue(function, result->type), likely_retain_mat(result->getData()));
    }

    Variant evaluateConstantFunction(const vector<likely_const_mat> &args = vector<likely_const_mat>()) const
    {
        vector<likely_matrix_type> params;
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
        return Variant(value, jit.returnType);
    }

    static void *getFunction(likely_const_expr expr, const vector<likely_matrix_type> &types)
    {
        if (!expr || (expr->uid() != UID()))
            return NULL;
        const Lambda *lambda = static_cast<const Lambda*>(expr);
        JITFunction *jitFunction = new JITFunction("likely_jit_function", lambda, types, false, false);
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
            likely_const_expr arg = get(builder, ast->atoms[i+1]);
            if (!arg)
                goto cleanup;

            if ((builder.env->type & likely_environment_ctfe) && (constantArgs.size() == args.size()))
                if (likely_const_mat constantArg = arg->getData())
                    constantArgs.push_back(constantArg);

            args.push_back(arg);
        }

        result = ((builder.env->type & likely_environment_ctfe)
                  && (constantArgs.size() == args.size())) ? ConstantMat::get(builder, likely_retain_mat(evaluateConstantFunction(constantArgs)))
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
            builder.module->exprs.push_back(vtable);

            PointerType *vTableType = PointerType::getUnqual(StructType::create(builder.getContext(), "VTable"));
            Function *likelyDynamic = builder.module->module->getFunction("likely_dynamic");
            if (!likelyDynamic) {
                Type *params[] = { vTableType, PointerType::get(builder.multiDimension(), 0) };
                FunctionType *likelyDynamicType = FunctionType::get(builder.multiDimension(), params, false);
                likelyDynamic = Function::Create(likelyDynamicType, GlobalValue::ExternalLinkage, "likely_dynamic", builder.module->module);
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
            return new likely_expression(LikelyValue(builder.CreateCall(likelyDynamic, args), likely_matrix_multi_dimension));
        }

        if (parameters) {
            if (parameters->type == likely_ast_list) {
                for (size_t i=0; i<args.size(); i++)
                    define(builder.env, parameters->atoms[i]->atom, args[i]);
            } else {
                define(builder.env, parameters->atom, args[0]);
            }
        }
        likely_const_expr result = get(builder, body);
        if (parameters)
            undefineAll(builder.env, parameters, false);
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
    size_t maxParameters() const { return 3; }

    static likely_matrix_type typeFromAst(Builder &builder, likely_const_ast ast, bool *ok)
    {
        const unique_ptr<const likely_expression> expr(get(builder, ast));
        if (!expr) {
            *ok = false;
            return likely_matrix_void;
        }

        const likely_const_mat data = expr->getData();
        if (!data || (data->type != likely_matrix_u32) || (data->type & likely_matrix_multi_dimension)) {
            *ok = false;
            return likely_matrix_void;
        }

        *ok = true;
        return likely_matrix_type(likely_element(data, 0, 0, 0, 0));
    }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        bool ok;
        const likely_matrix_type returnType = typeFromAst(builder, ast->atoms[1], &ok);
        if (!ok)
            return NULL;

        string name;
        {
            TRY_EXPR(builder, ast->atoms[2], expr);
            const likely_const_mat data = expr->getData();
            if (!likely_is_string(data))
                return NULL;
            name = data->data;
        }

        vector<likely_matrix_type> parameters;
        if (ast->atoms[3]->type == likely_ast_list) {
            for (uint32_t i=0; i<ast->atoms[3]->num_atoms; i++) {
                parameters.push_back(typeFromAst(builder, ast->atoms[3]->atoms[i], &ok));
                if (!ok)
                    return NULL;
            }
        } else {
            parameters.push_back(typeFromAst(builder, ast->atoms[3], &ok));
            if (!ok)
                return NULL;
        }

        return new Symbol(name, returnType, parameters);
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
        : likely_expression(LikelyValue(basicBlock, likely_matrix_void)) {}

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
            const likely_matrix_type resolved = likely_type_from_types(*t, *f);

            builder.SetInsertPoint(True);
            const likely_expression tc(builder.cast(*t, resolved));
            builder.CreateBr(End);

            builder.SetInsertPoint(False);
            const likely_expression fc(builder.cast(*f, resolved));
            builder.CreateBr(End);

            builder.SetInsertPoint(End);
            PHINode *phi = builder.CreatePHI(builder.toLLVM(resolved), 2);
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
        define(builder.env, ast->atoms[2]->atom, &loop);
        likely_const_expr expression = get(builder, ast->atoms[1]);
        undefine(builder.env, ast->atoms[2]->atom);
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
    struct KernelArgument : public Assignable
    {
        const string name;
        MDNode *node = NULL;
        Value *channels, *columns, *rows, *frames;
        Value *rowStep, *frameStep;

        KernelArgument(Builder &builder, const likely_expression &matrix, const string &name)
            : Assignable(matrix.value, matrix.type), name(name)
        {
            channels   = builder.channels(*this);
            columns    = builder.columns(*this);
            rowStep    = builder.multiplyInts(columns, channels);
            rows       = builder.rows(*this);
            frameStep  = builder.multiplyInts(rows, rowStep);
            frames     = builder.frames(*this);
            channels ->setName(name + "_c");
            columns  ->setName(name + "_x");
            rows     ->setName(name + "_y");
            frames   ->setName(name + "_t");
            rowStep  ->setName(name + "_y_step");
            frameStep->setName(name + "_t_step");
        }

    private:
        Value *gep(Builder &builder, likely_const_ast) const
        {
            Value *i = builder.zero();
            if (type & likely_matrix_multi_channel) i = likely_lookup(builder.env, "c")->expr->value;
            if (type & likely_matrix_multi_column ) i = builder.addInts(builder.multiplyInts(likely_lookup(builder.env, "x")->expr->value, channels ), i);
            if (type & likely_matrix_multi_row    ) i = builder.addInts(builder.multiplyInts(likely_lookup(builder.env, "y")->expr->value, rowStep  ), i);
            if (type & likely_matrix_multi_frame  ) i = builder.addInts(builder.multiplyInts(likely_lookup(builder.env, "t")->expr->value, frameStep), i);
            return builder.CreateGEP(builder.data(*this), i);
        }

        void set(Builder &builder, likely_const_expr expr, likely_const_ast ast) const
        {
            StoreInst *store = builder.CreateStore(builder.cast(*expr, type), gep(builder, ast));
            store->setMetadata("llvm.mem.parallel_loop_access", node);
        }

        likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
        {
            if (ast->type == likely_ast_list)
                return error(ast, "kernel operator does not take arguments");

            if (!isa<PointerType>(value->getType()))
                return new likely_expression((LikelyValue) *this);

            LoadInst *load = builder.CreateLoad(gep(builder, ast));
            load->setMetadata("llvm.mem.parallel_loop_access", node);
            return new likely_expression(LikelyValue(load, type & likely_matrix_element));
        }
    };

    struct KernelAxis : public Loop
    {
        KernelAxis *parent, *child;
        MDNode *node;
        Value *offset;

        KernelAxis(Builder &builder, const string &name, Value *start, Value *stop, Value *step, KernelAxis *parent)
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
        if      (srcs[0]->type & likely_matrix_multi_frame)   kernelSize = builder.frames(*srcs[0]);
        else if (srcs[0]->type & likely_matrix_multi_row)     kernelSize = builder.rows(*srcs[0]);
        else if (srcs[0]->type & likely_matrix_multi_column)  kernelSize = builder.columns(*srcs[0]);
        else if (srcs[0]->type & likely_matrix_multi_channel) kernelSize = builder.channels(*srcs[0]);
        else                                                  kernelSize = builder.one();

        if      (builder.env->type & likely_environment_heterogeneous) generateHeterogeneous(builder, ast, srcs, kernelSize);
        else if (builder.env->type & likely_environment_parallel)      generateParallel     (builder, ast, srcs, kernelSize);
        else                                                           generateSerial       (builder, ast, srcs, kernelSize);

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
            Type *params[] = { PointerType::getUnqual(parameterStructType), builder.nativeInt(), builder.nativeInt() };
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

        Type *params[] = { thunk->getType(), PointerType::getUnqual(parameterStructType), builder.nativeInt() };
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

        KernelAxis *axis = NULL;
        for (int axis_index=0; axis_index<4; axis_index++) {
            string name;
            bool multiElement;
            Value *elements, *step;

            switch (axis_index) {
              case 0:
                name = "t";
                multiElement = (kernelArguments[0]->type & likely_matrix_multi_frame) != 0;
                elements = kernelArguments[0]->frames;
                step = kernelArguments[0]->frameStep;
                break;
              case 1:
                name = "y";
                multiElement = (kernelArguments[0]->type & likely_matrix_multi_row) != 0;
                elements = kernelArguments[0]->rows;
                step = kernelArguments[0]->rowStep;
                break;
              case 2:
                name = "x";
                multiElement = (kernelArguments[0]->type & likely_matrix_multi_column) != 0;
                elements = kernelArguments[0]->columns;
                step = kernelArguments[0]->channels;
                break;
              default:
                name = "c";
                multiElement = (kernelArguments[0]->type & likely_matrix_multi_channel) != 0;
                elements = kernelArguments[0]->channels;
                step = builder.constant(1);
                break;
            }

            if (multiElement || ((axis_index == 3) && !axis)) {
                if (!axis) axis = new KernelAxis(builder, name, start, stop, step, NULL);
                else       axis = new KernelAxis(builder, name, builder.zero(), elements, step, axis);
                define(builder.env, name.c_str(), axis); // takes ownership of axis
            } else {
                define(builder.env, name.c_str(), new likely_expression(LikelyValue(builder.zero(), likely_matrix_native)));
            }
        }

        for (KernelArgument *kernelArgument : kernelArguments) {
            kernelArgument->node = axis->node;
            define(builder.env, kernelArgument->name.c_str(), kernelArgument);
        }

        delete get(builder, ast->atoms[2]);

        undefineAll(builder.env, args, true);
        kernelArguments.clear();

        axis->close(builder);
        delete undefine(builder.env, "c");
        delete undefine(builder.env, "x");
        delete undefine(builder.env, "y");
        delete undefine(builder.env, "t");
    }
};
LIKELY_REGISTER(kernel)

class defineExpression : public LikelyOperator
{
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
            return op.get() ? op->evaluate(builder, ast) : NULL;
        }

    public:
        LazyDefinition(likely_const_env env, likely_const_ast ast)
            : env(env), ast(ast) {}
    };

    const char *symbol() const { return "="; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast lhs = ast->atoms[1];
        likely_const_ast rhs = ast->atoms[2];
        const char *name = likely_symbol(ast);

        if (!builder.module /* global variable */) {
            builder.module = builder.env->module;
            if (lhs->type == likely_ast_list) {
                // Export symbol
                vector<likely_matrix_type> parameters;
                for (size_t i=1; i<lhs->num_atoms; i++) {
                    if (lhs->atoms[i]->type == likely_ast_list)
                        return error(lhs->atoms[i], "expected an atom name parameter type");
                    parameters.push_back(likely_type_from_string(lhs->atoms[i]->atom, NULL));
                }

                if (builder.env->module /* static compilation */) {
                    TRY_EXPR(builder, rhs, expr);
                    const Lambda *lambda = static_cast<const Lambda*>(expr.get());
                    if (likely_const_expr function = lambda->generate(builder, parameters, name, false, false)) {
                        likely_const_expr expr = new Symbol(name, function->type, parameters);
                        delete function;
                        return expr;
                    }
                } else {
                    JITFunction *function = new JITFunction(name, unique_ptr<Lambda>(new Lambda(builder.env, rhs->atoms[2], rhs->atoms[1])).get(), parameters, false, false);
                    if (function->function) {
                        sys::DynamicLibrary::AddSymbol(name, function->function);
                        return function;
                    } else {
                        delete function;
                    }
                }
            } else {
                if (!strcmp(likely_symbol(rhs), "->")) {
                    // Global variable
                    return get(builder, rhs);
                } else {
                    // Lazy global value
                    return new LazyDefinition(builder.env, rhs);
                }
            }
        } else {
            likely_const_expr expr = get(builder, rhs);
            define(builder.env, name, expr);
            return new likely_expression((LikelyValue) *expr);
        }
        return NULL;
    }
};
LIKELY_REGISTER(define)

class setExpression : public LikelyOperator
{
    struct Variable : public Assignable
    {
        Variable(Builder &builder, likely_const_expr expr, const string &name)
            : Assignable(builder.CreateAlloca(builder.toLLVM(expr->type), 0, name), *expr)
        {
            set(builder, expr);
        }

    private:
        void set(Builder &builder, likely_const_expr expr, likely_const_ast = NULL) const
        {
            builder.CreateStore((type & likely_matrix_multi_dimension) ? expr->value
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
            const likely_const_env env = likely_lookup(builder.env, name);
            const Assignable *assignable = env ? Assignable::dynamicCast(env->expr) : NULL;
            if (assignable) assignable->set(builder, expr, lhs);
            else            define(builder.env, name, new Variable(builder, expr, name));
        }
        return expr;
    }
};
LIKELY_REGISTER(set)

class evalExpression : public LikelyOperator
{
    const char *symbol() const { return "eval"; }
    size_t maxParameters() const { return 1; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[1], expr);
        const likely_const_mat source = expr->getData();
        if (!likely_is_string(source))
            return error(ast->atoms[1], "expected source code");

        const likely_const_env parent = builder.env;
        const likely_ast source_ast = likely_lex_and_parse(source->data, likely_file_gfm);
        builder.env = likely_eval(source_ast, parent, NULL, NULL);
        likely_release_ast(source_ast);
        likely_release_env(parent);
        if (builder.env->expr) return new likely_expression();
        else                   return NULL;
    }
};
LIKELY_REGISTER(eval)

JITFunction::JITFunction(const string &name, const Lambda *lambda, const vector<likely_matrix_type> &parameters, bool evaluate, bool arrayCC)
    : Symbol(name, likely_matrix_void, parameters), module(new likely_module())
{
    likely_env env = newEnv(lambda->env);

    // Don't do compile time function evaluation when we're only interested in the result.
    if (evaluate)
        env->type &= ~likely_environment_ctfe;

    Builder builder(env, module);
    {
        unique_ptr<const likely_expression> expr(lambda->generate(builder, parameters, name, arrayCC, evaluate));
        if (expr) {
            value = expr->value;
            type = expr->type;
            setData(likely_retain_mat(expr->getData()));
        }
    }
    likely_release_env(env);
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
    engineBuilder.setErrorStr(&error);

    if (evaluate) {
        engineBuilder.setEngineKind(EngineKind::Interpreter);
    } else {
        engineBuilder.setEngineKind(EngineKind::JIT)
                     .setUseMCJIT(true);
    }

    EE = engineBuilder.create(targetMachine);
    likely_assert(EE != NULL, "failed to create execution engine with error: %s", error.c_str());

    if (!evaluate) {
        EE->setObjectCache(&TheJITFunctionCache);
        if (!TheJITFunctionCache.alert(builder.module->module))
            builder.module->optimize();

        EE->finalizeObject();
        function = (void*) EE->getFunctionAddress(name);

        // cleanup
        EE->removeModule(builder.module->module);
        builder.module->finalize();
    }
//    builder.module->module->dump();
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
            case 6: data     = unique_ptr<const likely_expression>(get(builder, ast->atoms[6]))->value;
            case 5: frames   = builder.cast(*unique_ptr<const likely_expression>(get(builder, ast->atoms[5])).get(), likely_matrix_u32);
            case 4: rows     = builder.cast(*unique_ptr<const likely_expression>(get(builder, ast->atoms[4])).get(), likely_matrix_u32);
            case 3: columns  = builder.cast(*unique_ptr<const likely_expression>(get(builder, ast->atoms[3])).get(), likely_matrix_u32);
            case 2: channels = builder.cast(*unique_ptr<const likely_expression>(get(builder, ast->atoms[2])).get(), likely_matrix_u32);
            case 1: type     = builder.cast(*unique_ptr<const likely_expression>(get(builder, ast->atoms[1])).get(), likely_matrix_u32);
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

        return new likely_expression(LikelyValue(builder.CreatePointerCast(builder.newMat(type, channels, columns, rows, frames, data), builder.toLLVM(inferredType)), inferredType));
    }

    static void checkDimension(likely_matrix_type &type, Value *dimension, likely_matrix_type mask)
    {
        if (ConstantInt *dim = dyn_cast<ConstantInt>(dimension))
            if (dim->getZExtValue() == 1)
                type &= ~mask;
    }
};
LIKELY_REGISTER(new)

} // namespace (anonymous)

// As a special exception, this function is allowed to set ast->type
likely_const_expr likely_expression::get(Builder &builder, likely_const_ast ast)
{
    if (ast->type == likely_ast_list) {
        if (ast->num_atoms == 0)
            return likely_expression::error(ast, "Empty expression");
        likely_const_ast op = ast->atoms[0];
        if (op->type != likely_ast_list)
            if (const likely_const_env e = likely_lookup(builder.env, op->atom))
                return e->expr->evaluate(builder, ast);
        TRY_EXPR(builder, op, e);
        return e->evaluate(builder, ast);
    } else {
        if (const likely_const_env e = likely_lookup(builder.env, ast->atom)) {
            const_cast<likely_ast>(ast)->type = likely_ast_operator;
            return e->expr->evaluate(builder, ast);
        }

        if ((ast->atom[0] == '"') && (ast->atom[ast->atom_len-1] == '"')) {
            const_cast<likely_ast>(ast)->type = likely_ast_string;
            return new likely_expression(LikelyValue(builder.CreateGlobalStringPtr(string(ast->atom).substr(1, ast->atom_len-2)), likely_matrix_i8 | likely_matrix_pointer));
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
            const likely_matrix_type matrixType = likely_type_from_string(ast->atom, &ok);
            if (ok) {
                const_cast<likely_ast>(ast)->type = likely_ast_type;
                return new MatrixType(builder, matrixType);
            }

            const likely_file_type fileType = likely_file_type_from_string(ast->atom, &ok);
            if (ok) {
                const_cast<likely_ast>(ast)->type = likely_ast_type;
                return new likely_expression(LikelyValue(builder.constant(uint64_t(fileType), likely_matrix_u32)));
            }
        }

        const_cast<likely_ast>(ast)->type = likely_ast_invalid;
        return likely_expression::error(ast, "invalid literal");
    }
}

likely_env likely_standard(const char *file_name)
{
    likely_env env = newEnv(RootEnvironment::get());
    if (file_name)
        env->module = new OfflineModule(file_name);
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

    delete env->expr;
    likely_release_ast(env->ast);
    if (env->module && !env->parent->module)
        delete env->module;
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
        vtable->functions.push_back(unique_ptr<JITFunction>(new JITFunction("likely_vtable_entry", unique_ptr<Lambda>(new Lambda(vtable->env, vtable->body, vtable->parameters)).get(), types, false, true)));
        function = vtable->functions.back()->function;
        if (function == NULL)
            return NULL;
    }

    return reinterpret_cast<likely_mat (*)(likely_const_mat const*)>(function)(mats);
}

void *likely_compile(likely_const_expr expr, likely_matrix_type const *type, uint32_t n)
{
    return Lambda::getFunction(expr, vector<likely_matrix_type>(type, type + n));
}

likely_const_mat likely_result(likely_const_env env)
{
    if (!env || !env->expr)
        return NULL;
    return env->expr->getData();
}

likely_env likely_eval(likely_ast ast, likely_const_env parent, likely_eval_callback eval_callback, void *context)
{
    if (!ast || (ast->type != likely_ast_list))
        return NULL;

    likely_env env = likely_retain_env(parent);
    for (size_t i=0; i<ast->num_atoms; i++) {
        const likely_ast statement = ast->atoms[i];
        if (!statement)
            continue;

        Builder builder(parent, parent->module);
        const bool definition = (statement->type == likely_ast_list) && (statement->num_atoms > 0) && !strcmp(statement->atoms[0]->atom, "=");
        likely_const_expr expr = NULL;
        if (definition) {
            builder.module = NULL; // signify global scope
            expr = likely_expression::get(builder, statement);
        } else {
            // If `ast` is not a lambda then it is a computation we perform by constructing and executing a parameterless lambda.
            if (!strcmp(likely_symbol(statement), "->"))
                expr = likely_expression::get(builder, statement);
            else
                expr = ConstantMat::get(likely_retain_mat(Lambda(parent, statement).evaluateConstantFunction()));
        }

        // Certain operators like `eval` introduce additional to variables to the environment,
        // therefore, `builer.env` is not necessarily equal to `parent`.
        env = newEnv(builder.env);
        if (definition)
            env->type |= likely_environment_definition;
        env->ast = likely_retain_ast(statement);
        env->expr = expr;

        likely_release_env(parent);
        parent = env;
        if (eval_callback)
            eval_callback(env, context);
        if (!env->expr)
            break;
    }

    return env;
}

//! [likely_lookup implementation.]
likely_const_env likely_lookup(likely_const_env env, const char *name)
{
    while (env) {
        if ((env->type & likely_environment_definition) && !strcmp(name, likely_symbol(env->ast)))
            return env;
        env = env->parent;
    }
    return NULL;
}
//! [likely_lookup implementation.]

void likely_shutdown()
{
    likely_release_env(RootEnvironment::get());
    LikelyContext::shutdown();
    llvm_shutdown();
}
