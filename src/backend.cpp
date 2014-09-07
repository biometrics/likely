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

using namespace llvm;
using namespace std;

namespace {

struct Builder;

} // namespace (anonymous)

struct likely_expression
{
    Value *value;
    likely_type type;
    likely_const_expr parent;
    vector<likely_const_expr> subexpressions;

    likely_expression(Value *value = NULL, likely_type type = likely_matrix_void, likely_const_expr parent = NULL)
        : value(value), type(type), parent(parent) {}

    virtual ~likely_expression()
    {
        for (likely_const_expr e : subexpressions)
            delete e;
    }

    virtual int uid() const { return 0; }
    virtual size_t maxParameters() const { return 0; }
    virtual size_t minParameters() const { return maxParameters(); }
    virtual void *symbol() const { return NULL; } // Idiom to ensure that specified library symbols aren't stripped when optimizing executable size

    virtual likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const;

    operator Value*() const { return value; }
    operator likely_type() const { return type; }

    void dump() const
    {
        likely_const_mat m = likely_type_to_string(type);
        cerr << m->data << " ";
        value->dump();
        likely_release(m);
    }

    Value *take() const
    {
        Value *result = value;
        delete this; // With great power comes great responsibility
        return result;
    }

    vector<likely_const_expr> subexpressionsOrSelf() const
    {
        if (subexpressions.empty()) {
            vector<likely_const_expr> expressions;
            expressions.push_back(this);
            return expressions;
        } else {
            return subexpressions;
        }
    }

    static likely_const_expr error(likely_const_ast ast, const char *message)
    {
        likely_throw(ast, message);
        return NULL;
    }

    static likely_type validFloatType(likely_type type)
    {
        likely_set_floating(&type, true);
        likely_set_signed(&type, true);
        likely_set_depth(&type, likely_depth(type) > 32 ? 64 : 32);
        return type;
    }

    static likely_const_expr lookup(likely_const_env env, const char *name)
    {
        if (!env)
            return NULL;
        if (likely_definition(env->type) && !strcmp(name, likely_get_symbol_name(env->ast)))
            return env->value;
        return lookup(env->parent, name);
    }

    static void define(likely_env &env, const char *name, likely_const_expr value)
    {
        env = likely_new_env(env);
        likely_set_definition(&env->type, true);
        env->ast = likely_new_atom(name, strlen(name));
        env->value = value;
    }

    static likely_const_expr undefine(likely_env &env, const char *name)
    {
        assert(likely_definition(env->type));
        likely_assert(!strcmp(name, likely_get_symbol_name(env->ast)), "undefine variable mismatch");
        likely_const_expr value = env->value;
        env->value = NULL;
        likely_env old = env;
        env = const_cast<likely_env>(env->parent);
        likely_release_env(old);
        return value;
    }

    static size_t length(likely_const_ast ast)
    {
        return (ast->type == likely_ast_list) ? ast->num_atoms : 1;
    }

    static bool isMat(Type *type)
    {
        // This is safe because matricies are the only struct types created by the backend
        if (PointerType *ptr = dyn_cast<PointerType>(type))
            if (dyn_cast<StructType>(ptr->getElementType()))
                return true;
        return false;
    }

    static likely_type toLikely(Type *llvm)
    {
        if      (llvm->isIntegerTy()) return llvm->getIntegerBitWidth();
        else if (llvm->isHalfTy())    return likely_matrix_f16;
        else if (llvm->isFloatTy())   return likely_matrix_f32;
        else if (llvm->isDoubleTy())  return likely_matrix_f64;
        else                          return likely_type_from_string(cast<StructType>(cast<PointerType>(llvm)->getElementType())->getName().str().c_str());
    }
};

namespace {

#define TRY_EXPR(BUILDER, AST, EXPR)                                       \
const unique_ptr<const likely_expression> EXPR((BUILDER).expression(AST)); \
if (!EXPR.get()) return NULL;                                              \

} // namespace (anonymous)

class LikelyContext
{
    static queue<LikelyContext*> contextPool;
    map<likely_type, Type*> typeLUT;
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
            likely_assert(sizeof(likely_size) == sizeof(void*), "insane type system");
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

    Type *scalar(likely_type type, bool pointer = false)
    {
        const size_t bits = likely_depth(type);
        const bool floating = likely_floating(type);
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

    Type *toLLVM(likely_type likely)
    {
        auto result = typeLUT.find(likely);
        if (result != typeLUT.end())
            return result->second;

        Type *llvm;
        if (!likely_multi_dimension(likely) && likely_depth(likely)) {
            llvm = scalar(likely);
        } else {
            likely_mat str = likely_type_to_string(likely);
            llvm = PointerType::getUnqual(StructType::create(str->data,
                                                             nativeInt(), // bytes
                                                             nativeInt(), // ref_count
                                                             nativeInt(), // channels
                                                             nativeInt(), // columns
                                                             nativeInt(), // rows
                                                             nativeInt(), // frames
                                                             nativeInt(), // type
                                                             ArrayType::get(Type::getInt8Ty(context), 0), // data
                                                             NULL));
            likely_release(str);
        }

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

struct likely_module
{
    LikelyContext *context;
    Module *module;
    vector<likely_const_expr> expressions;

    likely_module()
        : context(LikelyContext::acquire())
        , module(new Module("likely_module", context->context)) {}

    virtual ~likely_module()
    {
        finalize();
        for (likely_const_expr e : expressions)
            delete e;
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

class JITFunctionCache : public ObjectCache
{
    map<hash_code, unique_ptr<MemoryBuffer>> cachedModules;
    map<const Module*, hash_code> currentModules;
    mutex editLock;

    void notifyObjectCompiled(const Module *M, MemoryBufferRef Obj)
    {
        lock_guard<mutex> lock(editLock);
        const auto currentModule = currentModules.find(M);
        const hash_code hash = currentModule->second;
        currentModules.erase(currentModule);
        cachedModules[hash] = MemoryBuffer::getMemBufferCopy(Obj.getBuffer());
    }

    unique_ptr<MemoryBuffer> getObject(const Module *M)
    {
        lock_guard<mutex> lock(editLock);
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
        const bool hit = (cachedModules.find(hash) != cachedModules.end());
        lock_guard<mutex> lock(editLock);
        currentModules.insert(pair<const Module*, hash_code>(M, hash));
        return hit;
    }
};
static JITFunctionCache TheJITFunctionCache;

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

    static likely_const_expr getMat(likely_const_expr e)
    {
        if (!e) return NULL;
        if (e->value)
            if (PointerType *type = dyn_cast<PointerType>(e->value->getType()))
                if (isa<StructType>(type->getElementType()))
                    return e;
        return getMat(e->parent);
    }

    likely_expression constant(uint64_t value, likely_type type = likely_matrix_native)
    {
        const unsigned depth = unsigned(likely_depth(type));
        return likely_expression(Constant::getIntegerValue(Type::getIntNTy(getContext(), depth), APInt(depth, value)), type);
    }

    likely_expression constant(double value, likely_type type)
    {
        const size_t depth = likely_depth(type);
        if (likely_floating(type)) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return likely_expression(ConstantFP::get(Type::getDoubleTy(getContext()), value), type);
            else if (depth == 32) return likely_expression(ConstantFP::get(Type::getFloatTy(getContext()), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return likely_expression(NULL, likely_matrix_void); }
        } else {
            return constant(uint64_t(value), type);
        }
    }

    likely_expression zero(likely_type type = likely_matrix_native) { return constant(0.0, type); }
    likely_expression one (likely_type type = likely_matrix_native) { return constant(1.0, type); }
    likely_expression intMax(likely_type type) { const size_t bits = likely_depth(type); return constant((uint64_t) (1 << (bits - (likely_signed(type) ? 1 : 0)))-1, bits); }
    likely_expression intMin(likely_type type) { const size_t bits = likely_depth(type); return constant((uint64_t) (likely_signed(type) ? (1 << (bits - 1)) : 0), bits); }
    likely_expression typeType(likely_type type) { return constant((uint64_t) type, likely_matrix_type_type); }
    likely_expression nullMat() { return likely_expression(ConstantPointerNull::get(::cast<PointerType>((Type*)multiDimension())), likely_matrix_void); }
    likely_expression nullData() { return likely_expression(ConstantPointerNull::get(Type::getInt8PtrTy(getContext())), likely_matrix_native); }

    likely_expression channels(likely_const_expr e) { likely_const_expr m = getMat(e); return (m && likely_multi_channel(*m)) ? likely_expression(CreateLoad(CreateStructGEP(*m, 2), "channels"), likely_matrix_native) : one(); }
    likely_expression columns (likely_const_expr e) { likely_const_expr m = getMat(e); return (m && likely_multi_column (*m)) ? likely_expression(CreateLoad(CreateStructGEP(*m, 3), "columns" ), likely_matrix_native) : one(); }
    likely_expression rows    (likely_const_expr e) { likely_const_expr m = getMat(e); return (m && likely_multi_row    (*m)) ? likely_expression(CreateLoad(CreateStructGEP(*m, 4), "rows"    ), likely_matrix_native) : one(); }
    likely_expression frames  (likely_const_expr e) { likely_const_expr m = getMat(e); return (m && likely_multi_frame  (*m)) ? likely_expression(CreateLoad(CreateStructGEP(*m, 5), "frames"  ), likely_matrix_native) : one(); }
    likely_expression data    (likely_const_expr e) { likely_const_expr m = getMat(e); return likely_expression(CreatePointerCast(CreateStructGEP(*m, 7), env->module->context->scalar(*m, true)), likely_data(*m)); }

    void steps(likely_const_expr matrix, Value *channelStep, Value **columnStep, Value **rowStep, Value **frameStep)
    {
        *columnStep = CreateMul(channels(matrix), channelStep, "x_step");
        *rowStep    = CreateMul(columns(matrix), *columnStep, "y_step");
        *frameStep  = CreateMul(rows(matrix), *rowStep, "t_step");
    }

    likely_expression cast(likely_const_expr x, likely_type type)
    {
        if (likely_data(*x) == likely_data(type))
            return likely_expression(*x, type);
        if (likely_depth(type) == 0) {
            likely_set_depth(&type, likely_depth(*x));
            if (likely_floating(type))
                type = likely_expression::validFloatType(type);
        }
        Type *dstType = env->module->context->scalar(type);
        return likely_expression(CreateCast(CastInst::getCastOpcode(*x, likely_signed(*x), dstType, likely_signed(type)), *x, dstType), type);
    }

    likely_const_expr lookup(const char *name) const { return likely_expression::lookup(env, name); }
    void define(const char *name, likely_const_expr e) { likely_expression::define(env, name, e); }
    likely_const_expr undefine(const char *name) { return likely_expression::undefine(env, name); }

    void undefineAll(likely_const_ast args, bool deleteExpression)
    {
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

    IntegerType *nativeInt() { return env->module->context->nativeInt(); }
    Type *multiDimension() { return toLLVM(likely_matrix_multi_dimension); }
    Module *module() { return env->module->module; }
    Type *toLLVM(likely_type likely) { return env->module->context->toLLVM(likely); }
    Type *translate(Type *type) { return toLLVM(likely_expression::toLikely(type)); } // Translate type across contexts

    likely_const_expr expression(likely_const_ast ast);
};

struct Symbol : public likely_expression
{
    Symbol(Function *function = NULL)
        : likely_expression(function, function ? toLikely(function->getReturnType()) : likely_matrix_void) {}

private:
    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        Function *definition = cast<Function>(value);
        if (definition->arg_size() != ((ast->type == likely_ast_list) ? ast->num_atoms-1 : 0))
            return error(ast, "incorrect argument count");

        Function *symbol = builder.module()->getFunction(definition->getName());
        if (!symbol) {
            // Translate definition type across contexts
            vector<Type*> paramTypes;
            Function::arg_iterator it = definition->arg_begin();
            while (it != definition->arg_end()) {
                paramTypes.push_back(builder.translate(it->getType()));
                it++;
            }
            FunctionType *functionType = FunctionType::get(builder.translate(definition->getReturnType()), paramTypes, false);
            symbol = Function::Create(functionType, GlobalValue::ExternalLinkage, definition->getName(), builder.module());
        }

        vector<Value*> args;
        if (ast->type == likely_ast_list) {
            Function::arg_iterator it = symbol->arg_begin();
            for (size_t i=1; i<ast->num_atoms; i++, it++) {
                TRY_EXPR(builder, ast->atoms[i], arg)
                args.push_back(builder.cast(arg.get(), toLikely(it->getType())));
            }
        }

        return new likely_expression(builder.CreateCall(symbol, args), type);
    }
};

struct JITFunction : public likely_function, public Symbol
{
    ExecutionEngine *EE = NULL;
    likely_env env;
    const vector<likely_type> parameters;

    JITFunction(const string &name, likely_const_ast ast, likely_const_env parent, const vector<likely_type> &parameters, bool abandon, bool interpreter, bool arrayCC);

    ~JITFunction()
    {
        if (EE) {
            EE->removeModule(env->module->module);
            delete EE;
        }
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

class Operator : public likely_expression
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
            stream << " parameters passed: " << args << " arguments";
            return error(ast, stream.str().c_str());
        }

        return evaluateOperator(builder, ast);
    }

    virtual likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const = 0;
};

struct ScopedExpression : public Operator
{
    likely_env env;
    likely_const_ast ast;

    ScopedExpression(likely_env env, likely_const_ast ast)
        : env(env), ast(ast) {}
};

} // namespace (anonymous)

likely_const_expr likely_expression::evaluate(Builder &builder, likely_const_ast ast) const
{
    if (ast->type == likely_ast_list) {
        likely_expr expression = new likely_expression();
        for (size_t i=0; i<ast->num_atoms; i++) {
            if (likely_const_expr e = builder.expression(ast->atoms[i])) {
                expression->subexpressions.push_back(e);
            } else {
                delete expression;
                return NULL;
            }
        }
        return expression;
    } else {
        return new likely_expression(value, type);
    }
}

struct likely_virtual_table : public ScopedExpression
{
    size_t n;
    vector<unique_ptr<JITFunction>> functions;

    likely_virtual_table(likely_env env, likely_const_ast ast)
        : ScopedExpression(env, ast), n(length(ast->atoms[1])) {}

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
            likely_ast ast = likely_ast_from_string(likely_standard_library, true);
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
        static likely_env root = likely_new_env(NULL);
        return root;
    }
};

template <class E>
struct RegisterExpression : public RootEnvironment
{
    RegisterExpression(const char *symbol)
    {
        likely_expr e = new E();
        likely_expression::define(builtins(), symbol, e);
    }
};
#define LIKELY_REGISTER_EXPRESSION(EXP, SYM) static struct RegisterExpression<EXP##Expression> Register##EXP##Expression(SYM);
#define LIKELY_REGISTER(EXP) LIKELY_REGISTER_EXPRESSION(EXP, #EXP)

class UnaryOperator : public Operator
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

struct MatrixType : public SimpleUnaryOperator
{
    likely_type t;
    MatrixType(Builder &builder, likely_type t)
        : t(t)
    {
        value = builder.typeType(t);
        type = likely_matrix_type_type;
    }

private:
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &x) const
    {
        return new likely_expression(builder.cast(x.get(), t));
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
    }
    const string op = ast->atom;

    if (likely_const_expr e = lookup(op.c_str())) {
        const_cast<likely_ast>(ast)->type = likely_ast_operator;
        return e->evaluate(*this, ast);
    }

    if ((op.front() == '"') && (op.back() == '"')) {
        const_cast<likely_ast>(ast)->type = likely_ast_string;
        return new likely_expression(CreateGlobalStringPtr(op.substr(1, op.length()-2)), likely_matrix_u8);
    }

    { // Is it a number?
        char *p;
        const double value = strtod(op.c_str(), &p);
        if (*p == 0) {
            const_cast<likely_ast>(ast)->type = likely_ast_number;
            return new likely_expression(constant(value, likely_type_from_value(value)));
        }
    }

    { // Is it a type?
        bool ok;
        likely_type type = likely_type_field_from_string(op.c_str(), &ok);
        if (ok) {
            const_cast<likely_ast>(ast)->type = likely_ast_type;
            return new MatrixType(*this, type);
        }
    }

    const_cast<likely_ast>(ast)->type = likely_ast_unknown;
    return likely_expression::error(ast, "unrecognized literal");
}

#define LIKELY_REGISTER_FIELD(FIELD)                                         \
class FIELD##Expression : public likely_expression                           \
{                                                                            \
    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const \
    {                                                                        \
        const size_t arguments = length(ast) - 1;                            \
        if (arguments == 0) {                                                \
            return new FIELD##Expression();                                  \
        } else if (arguments == 1) {                                         \
            TRY_EXPR(builder, ast->atoms[1], expr)                           \
            return new likely_expression(builder.FIELD(expr.get()));         \
        } else {                                                             \
            return error(ast, "expected 0 or 1 operand(s)");                 \
        }                                                                    \
    }                                                                        \
};                                                                           \
LIKELY_REGISTER(FIELD)                                                       \

LIKELY_REGISTER_FIELD(channels)
LIKELY_REGISTER_FIELD(columns)
LIKELY_REGISTER_FIELD(rows)
LIKELY_REGISTER_FIELD(frames)

class notExpression : public SimpleUnaryOperator
{
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(builder.CreateXor(builder.intMax(*arg), arg->value), *arg);
    }
};
LIKELY_REGISTER_EXPRESSION(not, "~")

class typeExpression : public SimpleUnaryOperator
{
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
        likely_expression xc(builder.cast(x.get(), validFloatType(*x)));
        return new likely_expression(builder.CreateCall(Intrinsic::getDeclaration(builder.module(), id(), xc.value->getType()), xc), xc);
    }
    virtual Intrinsic::ID id() const = 0;
};

#define LIKELY_REGISTER_UNARY_MATH(OP)                 \
class OP##Expression : public UnaryMathOperator        \
{                                                      \
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

class SimpleBinaryOperator : public Operator
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
        return evaluateArithmetic(builder, builder.cast(lhs.get(), type), builder.cast(rhs.get(), type));
    }
    virtual likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const = 0;
};

class SimpleArithmeticOperator : public ArithmeticOperator
{
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        return new likely_expression(evaluateSimpleArithmetic(builder, lhs, rhs), lhs);
    }
    virtual Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const = 0;
};

class addExpression : public SimpleArithmeticOperator
{
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        if (likely_floating(lhs)) {
            return builder.CreateFAdd(lhs, rhs);
        } else {
            if (likely_saturation(lhs)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), likely_signed(lhs) ? Intrinsic::sadd_with_overflow : Intrinsic::uadd_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, builder.zero(lhs)), builder.intMax(lhs), builder.intMin(lhs)) : builder.intMax(lhs).value;
                return builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0));
            } else {
                return builder.CreateAdd(lhs, rhs);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(add, "+")

class subtractExpression : public Operator
{
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

        likely_type type = likely_type_from_types(*expr1, *expr2);
        likely_expression lhs = builder.cast(expr1.get(), type);
        likely_expression rhs = builder.cast(expr2.get(), type);

        if (likely_floating(type)) {
            return new likely_expression(builder.CreateFSub(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), likely_signed(lhs) ? Intrinsic::ssub_with_overflow : Intrinsic::usub_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, builder.zero(lhs)), builder.intMax(lhs), builder.intMin(lhs)) : builder.intMin(lhs).value;
                return new likely_expression(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), type);
            } else {
                return new likely_expression(builder.CreateSub(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(subtract, "-")

class multiplyExpression : public SimpleArithmeticOperator
{
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        if (likely_floating(lhs)) {
            return builder.CreateFMul(lhs, rhs);
        } else {
            if (likely_saturation(lhs)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), likely_signed(lhs) ? Intrinsic::smul_with_overflow : Intrinsic::umul_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *zero = builder.zero(lhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateXor(builder.CreateICmpSGE(lhs, zero), builder.CreateICmpSGE(rhs, zero)), builder.intMin(lhs), builder.intMax(lhs)) : builder.intMax(lhs).value;
                return builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0));
            } else {
                return builder.CreateMul(lhs, rhs);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(multiply, "*")

class divideExpression : public SimpleArithmeticOperator
{
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &n, const likely_expression &d) const
    {
        if (likely_floating(n)) {
            return builder.CreateFDiv(n, d);
        } else {
            if (likely_signed(n)) {
                if (likely_saturation(n)) {
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
LIKELY_REGISTER_EXPRESSION(divide, "/")

class remainderExpression : public SimpleArithmeticOperator
{
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        return likely_floating(lhs) ? builder.CreateFRem(lhs, rhs)
                                    : (likely_signed(lhs) ? builder.CreateSRem(lhs, rhs)
                                                          : builder.CreateURem(lhs, rhs));
    }
};
LIKELY_REGISTER_EXPRESSION(remainder, "%")

#define LIKELY_REGISTER_LOGIC(OP, SYM)                                                                                  \
class OP##Expression : public SimpleArithmeticOperator                                                                  \
{                                                                                                                       \
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const \
    {                                                                                                                   \
        return builder.Create##OP(lhs, rhs.value);                                                                      \
    }                                                                                                                   \
};                                                                                                                      \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                     \

LIKELY_REGISTER_LOGIC(And , "&")
LIKELY_REGISTER_LOGIC(Or  , "|")
LIKELY_REGISTER_LOGIC(Xor , "^")
LIKELY_REGISTER_LOGIC(Shl , "<<")
LIKELY_REGISTER_LOGIC(LShr, "lshr")
LIKELY_REGISTER_LOGIC(AShr, "ashr")

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                              \
class OP##Expression : public ArithmeticOperator                                                                                         \
{                                                                                                                                        \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const             \
    {                                                                                                                                    \
        return new likely_expression(likely_floating(lhs) ? builder.CreateFCmpO##OP(lhs, rhs)                                            \
                                                          : (likely_signed(lhs) ? builder.CreateICmpS##OP(lhs, rhs)                      \
                                                                                : builder.CreateICmpU##OP(lhs, rhs)), likely_matrix_u1); \
    }                                                                                                                                    \
};                                                                                                                                       \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                                      \

LIKELY_REGISTER_COMPARISON(LT, "<")
LIKELY_REGISTER_COMPARISON(LE, "<=")
LIKELY_REGISTER_COMPARISON(GT, ">")
LIKELY_REGISTER_COMPARISON(GE, ">=")

#define LIKELY_REGISTER_EQUALITY(OP, SYM)                                                                                    \
class OP##Expression : public ArithmeticOperator                                                                             \
{                                                                                                                            \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const \
    {                                                                                                                        \
        return new likely_expression(likely_floating(lhs) ? builder.CreateFCmpO##OP(lhs, rhs)                                \
                                                          : builder.CreateICmp##OP(lhs, rhs), likely_matrix_u1);             \
    }                                                                                                                        \
};                                                                                                                           \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                          \

LIKELY_REGISTER_EQUALITY(EQ, "==")
LIKELY_REGISTER_EQUALITY(NE, "!=")

class BinaryMathOperator : public SimpleBinaryOperator
{
    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &x, const unique_ptr<const likely_expression> &n) const
    {
        const likely_type type = validFloatType(likely_type_from_types(*x, *n));
        const likely_expression xc(builder.cast(x.get(), type));
        const likely_expression nc(builder.cast(n.get(), type));
        return new likely_expression(builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), id(), xc.value->getType()), xc, nc), xc);
    }
    virtual Intrinsic::ID id() const = 0;
};

#define LIKELY_REGISTER_BINARY_MATH(OP)                \
class OP##Expression : public BinaryMathOperator       \
{                                                      \
    Intrinsic::ID id() const { return Intrinsic::OP; } \
};                                                     \
LIKELY_REGISTER(OP)                                    \

LIKELY_REGISTER_BINARY_MATH(pow)
LIKELY_REGISTER_BINARY_MATH(copysign)

class definedExpression : public Operator
{
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast name = ast->atoms[1];
        if (name->type == likely_ast_list)
            return error(name, "expected an atom");
        if (builder.lookup(name->atom)) return builder.expression(name);
        else                            return builder.expression(ast->atoms[2]);
    }
};
LIKELY_REGISTER_EXPRESSION(defined, "??")

class elementsExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_elements; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        Function *likelyElements = builder.module()->getFunction("likely_elements");
        if (!likelyElements) {
            FunctionType *functionType = FunctionType::get(builder.nativeInt(), builder.multiDimension(), false);
            likelyElements = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_elements", builder.module());
            likelyElements->setCallingConv(CallingConv::C);
            likelyElements->setDoesNotAlias(1);
            likelyElements->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyElements, *arg), likely_matrix_native);
    }
};
LIKELY_REGISTER(elements)

class bytesExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_bytes; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        Function *likelyBytes = builder.module()->getFunction("likely_bytes");
        if (!likelyBytes) {
            FunctionType *functionType = FunctionType::get(builder.nativeInt(), builder.multiDimension(), false);
            likelyBytes = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_bytes", builder.module());
            likelyBytes->setCallingConv(CallingConv::C);
            likelyBytes->setDoesNotAlias(1);
            likelyBytes->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyBytes, builder.CreatePointerCast(*arg, builder.multiDimension())), likely_matrix_native);
    }
};
LIKELY_REGISTER(bytes)

class newExpression : public Operator
{
    size_t maxParameters() const { return 6; }
    size_t minParameters() const { return 0; }
    void *symbol() const { return (void*) likely_new; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const size_t n = ast->num_atoms - 1;
        unique_ptr<const likely_expression> type;
        Value *channels = NULL, *columns = NULL, *rows = NULL, *frames = NULL, *data = NULL;
        switch (n) {
            case 6: data     = builder.expression(ast->atoms[6])->take();
            case 5: frames   = builder.expression(ast->atoms[5])->take();
            case 4: rows     = builder.expression(ast->atoms[4])->take();
            case 3: columns  = builder.expression(ast->atoms[3])->take();
            case 2: channels = builder.expression(ast->atoms[2])->take();
            case 1: type.reset(builder.expression(ast->atoms[1]));
            default:           break;
        }

        switch (maxParameters()-n) {
            case 6: type.reset(new likely_expression(builder.typeType(validFloatType(likely_matrix_native))));
            case 5: channels = builder.one();
            case 4: columns  = builder.one();
            case 3: rows     = builder.one();
            case 2: frames   = builder.one();
            case 1: data     = builder.nullData();
            default:           break;
        }

        return new likely_expression(createCall(builder, *type, channels, columns, rows, frames, data), likely_matrix_multi_dimension);
    }

public:
    static CallInst *createCall(Builder &builder, Value *type, Value *channels, Value *columns, Value *rows, Value *frames, Value *data)
    {
        Function *likelyNew = builder.module()->getFunction("likely_new");
        if (!likelyNew) {
            Type *params[] = { builder.nativeInt(), builder.nativeInt(), builder.nativeInt(), builder.nativeInt(), builder.nativeInt(), Type::getInt8PtrTy(builder.getContext()) };
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), params, false);
            likelyNew = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_new", builder.module());
            likelyNew->setCallingConv(CallingConv::C);
            likelyNew->setDoesNotAlias(0);
            likelyNew->setDoesNotAlias(6);
            likelyNew->setDoesNotCapture(6);
        }

        Value* args[] = { type, channels, columns, rows, frames, data };
        return builder.CreateCall(likelyNew, args);
    }
};
LIKELY_REGISTER(new)

class scalarExpression : public UnaryOperator
{
    void *symbol() const { return (void*) likely_scalar_va; }

    likely_const_expr evaluateUnary(Builder &builder, likely_const_ast arg) const
    {
        likely_const_expr argExpr = builder.expression(arg);
        if (!argExpr)
            return NULL;

        if (argExpr->value && isMat(argExpr->value->getType()))
            return argExpr;

        Function *likelyScalar = builder.module()->getFunction("likely_scalar_va");
        if (!likelyScalar) {
            Type *params[] = { builder.nativeInt(), Type::getDoubleTy(builder.getContext()) };
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), params, true);
            likelyScalar = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_scalar_va", builder.module());
            likelyScalar->setCallingConv(CallingConv::C);
            likelyScalar->setDoesNotAlias(0);
            sys::DynamicLibrary::AddSymbol("lle_X_likely_scalar_va", (void*) lle_X_likely_scalar_va);
        }

        vector<Value*> args;
        likely_type type = likely_matrix_void;
        for (likely_const_expr e : argExpr->subexpressionsOrSelf()) {
            args.push_back(builder.cast(e, likely_matrix_f64));
            type = likely_type_from_types(type, e->type);
        }
        args.push_back(ConstantFP::get(builder.getContext(), APFloat::getNaN(APFloat::IEEEdouble)));
        args.insert(args.begin(), builder.typeType(type));

        likely_expression result(builder.CreateCall(likelyScalar, args), *argExpr);
        delete argExpr;
        return new likely_expression(result);
    }

    static GenericValue lle_X_likely_scalar_va(FunctionType *, const vector<GenericValue> &Args)
    {
        vector<double> values;
        for (size_t i=1; i<Args.size()-1; i++)
            values.push_back(Args[i].DoubleVal);
        return GenericValue(likely_scalar_n(Args[0].IntVal.getZExtValue(), values.data(), values.size()));
    }
};
LIKELY_REGISTER(scalar)

class stringExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_string; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), likely_matrix_i8);
    }

public:
    static CallInst *createCall(Builder &builder, Value *string)
    {
        Function *likelyString = builder.module()->getFunction("likely_string");
        if (!likelyString) {
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), Type::getInt8PtrTy(builder.getContext()), false);
            likelyString = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_string", builder.module());
            likelyString->setCallingConv(CallingConv::C);
            likelyString->setDoesNotAlias(0);
            likelyString->setDoesNotAlias(1);
            likelyString->setDoesNotCapture(1);
        }
        return builder.CreateCall(likelyString, string);
    }
};
LIKELY_REGISTER(string)

class copyExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_copy; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), *arg);
    }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        Function *likelyCopy = builder.module()->getFunction("likely_copy");
        if (!likelyCopy) {
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), builder.multiDimension(), false);
            likelyCopy = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_copy", builder.module());
            likelyCopy->setCallingConv(CallingConv::C);
            likelyCopy->setDoesNotAlias(0);
            likelyCopy->setDoesNotAlias(1);
            likelyCopy->setDoesNotCapture(1);
        }
        return builder.CreateCall(likelyCopy, m);
    }
};
LIKELY_REGISTER(copy)

class retainExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_retain; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), *arg);
    }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        Function *likelyRetain = builder.module()->getFunction("likely_retain");
        if (!likelyRetain) {
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), builder.multiDimension(), false);
            likelyRetain = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_retain", builder.module());
            likelyRetain->setCallingConv(CallingConv::C);
            likelyRetain->setDoesNotAlias(0);
            likelyRetain->setDoesNotAlias(1);
            likelyRetain->setDoesNotCapture(1);
        }
        return builder.CreateCall(likelyRetain, m);
    }
};
LIKELY_REGISTER(retain)

class releaseExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_release; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), *arg);
    }

    // This is not publicly accessible because cleanup() is the only time release statements should be inserted
    static CallInst *createCall(Builder &builder, Value *m)
    {
        Function *likelyRelease = builder.module()->getFunction("likely_release");
        if (!likelyRelease) {
            FunctionType *functionType = FunctionType::get(Type::getVoidTy(builder.getContext()), builder.multiDimension(), false);
            likelyRelease = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_release", builder.module());
            likelyRelease->setCallingConv(CallingConv::C);
            likelyRelease->setDoesNotAlias(1);
            likelyRelease->setDoesNotCapture(1);
        }
        return builder.CreateCall(likelyRelease, m);
    }

public:
    // Release intermediate matricies allocated to avoid memory leaks
    static void cleanup(Builder &builder, Value *ret)
    {
        // Looking for matricies that were created through function calls
        // but not returned by this function.
        Function *function = builder.GetInsertBlock()->getParent();
        for (Function::iterator BB = function->begin(), BBE = function->end(); BB != BBE; ++BB)
            for (BasicBlock::iterator I = BB->begin(), IE = BB->end(); I != IE; ++I)
               if (CallInst *call = dyn_cast<CallInst>(I))
                   if (Function *function = call->getCalledFunction())
                       if (isMat(function->getReturnType()) && (call != ret))
                           releaseExpression::createCall(builder, call);
    }
};
LIKELY_REGISTER(release)

struct Lambda : public ScopedExpression
{
    Lambda(likely_env env, likely_const_ast ast)
        : ScopedExpression(env, ast) {}

    const Symbol *generate(Builder &builder, vector<likely_type> types, string name, bool arrayCC) const
    {
        size_t n;
        if ((ast->type == likely_ast_list) && (ast->num_atoms > 1))
            if (ast->atoms[1]->type == likely_ast_list) n = ast->atoms[1]->num_atoms;
            else                                        n = 1;
        else                                            n = 0;

        while (types.size() < n)
            types.push_back(likely_matrix_multi_dimension);

        vector<Type*> llvmTypes;
        if (arrayCC) {
            // Array calling convention - All arguments, which must be matrix pointers, come stored in an array for the convenience of likely_dynamic.
            llvmTypes.push_back(PointerType::get(builder.multiDimension(), 0));
        } else {
            for (const likely_type &t : types)
                llvmTypes.push_back(builder.toLLVM(t));
        }

        BasicBlock *originalInsertBlock = builder.GetInsertBlock();
        Function *tmpFunction = cast<Function>(builder.module()->getOrInsertFunction(name+"_tmp", FunctionType::get(Type::getVoidTy(builder.getContext()), llvmTypes, false)));
        BasicBlock *entry = BasicBlock::Create(builder.getContext(), "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        vector<likely_const_expr> arguments;
        if (arrayCC) {
            Value *argumentArray = tmpFunction->arg_begin();
            for (size_t i=0; i<types.size(); i++)
                arguments.push_back(new likely_expression(builder.CreateLoad(builder.CreateGEP(argumentArray, builder.constant(i))), types[i]));
        } else {
            Function::arg_iterator it = tmpFunction->arg_begin();
            size_t i = 0;
            while (it != tmpFunction->arg_end())
                arguments.push_back(new likely_expression(it++, types[i++]));
        }

        for (size_t i=0; i<arguments.size(); i++) {
            stringstream name; name << "arg_" << i;
            arguments[i]->value->setName(name.str());
        }

        likely_const_expr result = evaluateFunction(builder, arguments);
        for (likely_const_expr arg : arguments)
            delete arg;
        if (!result)
            return NULL;

        releaseExpression::cleanup(builder, result->value);
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
        delete result;

        if (originalInsertBlock)
            builder.SetInsertPoint(originalInsertBlock);
        return new Symbol(function);
    }

private:
    size_t maxParameters() const { return length(ast->atoms[1]); }
    void *symbol() const { return (void*) likely_dynamic; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_expr result = NULL;

        vector<likely_const_expr> args;
        const size_t arguments = length(ast)-1;
        for (size_t i=0; i<arguments; i++) {
            likely_const_expr arg = builder.expression(ast->atoms[i+1]);
            if (!arg)
                goto cleanup;
            args.push_back(arg);
        }
        result = evaluateFunction(builder, args);

    cleanup:
        for (likely_const_expr arg : args)
            delete arg;
        return result;
    }

    likely_const_expr evaluateFunction(Builder &builder, const vector<likely_const_expr> &args) const
    {
        assert(args.size() == length(ast->atoms[1]));

        // Do dynamic dispatch if the type isn't fully specified
        bool dynamic = false;
        for (likely_const_expr arg : args)
            dynamic |= (arg->type == likely_matrix_multi_dimension);

        if (dynamic) {
            likely_vtable vtable = new likely_virtual_table(builder.env, ast);
            builder.env->module->expressions.push_back(vtable);

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
            }

            Value *matricies = builder.CreateAlloca(builder.multiDimension(), builder.constant(args.size()));
            for (size_t i=0; i<args.size(); i++)
                builder.CreateStore(*args[i], builder.CreateGEP(matricies, builder.constant(i)));
            Value* args[] = { ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(vtable)), uintptr_t(vtable)), vTableType), matricies };
            return new likely_expression(builder.CreateCall(likelyDynamic, args), likely_matrix_multi_dimension);
        }

        return evaluateLambda(builder, args);
    }

    virtual likely_const_expr evaluateLambda(Builder &builder, const vector<likely_const_expr> &args) const
    {
        if (ast->atoms[1]->type == likely_ast_list) {
            for (size_t i=0; i<args.size(); i++)
                builder.define(ast->atoms[1]->atoms[i]->atom, args[i]);
        } else {
            builder.define(ast->atoms[1]->atom, args[0]);
        }
        likely_const_expr result = builder.expression(ast->atoms[2]);
        builder.undefineAll(ast->atoms[1], false);
        return result;
    }
};

class lambdaExpression : public Operator
{
    size_t maxParameters() const { return 2; }
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const { return new Lambda(builder.env, ast); }

public:
    static bool isLambda(likely_const_ast ast)
    {
        return (ast->type == likely_ast_list)
               && (ast->num_atoms > 0)
               && (ast->atoms[0]->type != likely_ast_list)
               && (!strcmp(ast->atoms[0]->atom, "->") || !strcmp(ast->atoms[0]->atom, "=>"));
    }
};
LIKELY_REGISTER_EXPRESSION(lambda, "->")

class beginExpression : public Operator
{
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
LIKELY_REGISTER_EXPRESSION(begin, "{")

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

class labelExpression : public Operator
{
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
LIKELY_REGISTER_EXPRESSION(label, "#")

class ifExpression : public Operator
{
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
            likely_expression tc = builder.cast(t.get(), resolved);
            builder.CreateBr(End);

            builder.SetInsertPoint(False);
            likely_expression fc = builder.cast(f.get(), resolved);
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
LIKELY_REGISTER_EXPRESSION(if, "?")

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

class loopExpression : public Operator
{
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
LIKELY_REGISTER_EXPRESSION(loop, "$")

struct Kernel : public Lambda
{
    Kernel(likely_env env, likely_const_ast ast)
        : Lambda(env, ast) {}

private:
    class kernelArgument : public Operator
    {
        likely_type kernel;
        Value *channelStep;
        MDNode *node;

    public:
        kernelArgument(const likely_expression &matrix, likely_type kernel, Value *channelStep, MDNode *node)
            : kernel(kernel), channelStep(channelStep), node(node)
        {
            value = matrix.value;
            type = matrix.type;
        }

    private:
        size_t maxParameters() const { return 0; }

        likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
        {
            if (ast->type == likely_ast_list)
                return error(ast, "kernel operator does not take arguments");

            if (!isa<PointerType>(value->getType()))
                return new likely_expression(*this);

            Value *i;
            if (((type ^ kernel) & likely_matrix_multi_dimension) == 0) {
                // This matrix has the same dimensionality as the kernel
                i = *builder.lookup("i");
            } else {
                Value *columnStep, *rowStep, *frameStep;
                builder.steps(this, channelStep, &columnStep, &rowStep, &frameStep);
                i = builder.zero();
                if (likely_multi_channel(type)) i = builder.CreateMul(*builder.lookup("c"), channelStep);
                if (likely_multi_column (type)) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("x"), columnStep), i);
                if (likely_multi_row    (type)) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("y"), rowStep   ), i);
                if (likely_multi_frame  (type)) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("t"), frameStep ), i);
            }
            LoadInst *load = builder.CreateLoad(builder.CreateGEP(builder.data(this), i));

            load->setMetadata("llvm.mem.parallel_loop_access", node);
            return new likely_expression(load, type, this);
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
        size_t results;
    };

    void *symbol() const { return (void*) likely_fork; }

    virtual likely_const_ast getMetadata() const { return (ast->num_atoms == 4) ? ast->atoms[3] : NULL; }

    likely_const_expr evaluateLambda(Builder &builder, const vector<likely_const_expr> &srcs) const
    {
        const likely_const_ast args = ast->atoms[1];
        assert(srcs.size() == (args->type == likely_ast_list) ? args->num_atoms : 1);
        if (args->type == likely_ast_list) {
            for (size_t j=0; j<args->num_atoms; j++)
                builder.define(args->atoms[j]->atom, srcs[j]);
        } else {
            builder.define(args->atom, srcs[0]);
        }
        BasicBlock *entry = builder.GetInsertBlock();

        vector<pair<likely_const_ast,likely_const_ast>> pairs;
        getPairs(getMetadata(), pairs);

        likely_type dimensionsType = likely_matrix_void;
        Value *dstChannels = getDimensions(builder, pairs, "channels", srcs, &dimensionsType);
        Value *dstColumns  = getDimensions(builder, pairs, "columns" , srcs, &dimensionsType);
        Value *dstRows     = getDimensions(builder, pairs, "rows"    , srcs, &dimensionsType);
        Value *dstFrames   = getDimensions(builder, pairs, "frames"  , srcs, &dimensionsType);

        // Allocate and initialize memory for the destination matrix
        BasicBlock *allocation = BasicBlock::Create(builder.getContext(), "allocation", builder.GetInsertBlock()->getParent());
        builder.CreateBr(allocation);
        builder.SetInsertPoint(allocation);
        PHINode *results   = builder.CreatePHI(builder.nativeInt(), 1);
        PHINode *dstType   = builder.CreatePHI(builder.nativeInt(), 1);
        PHINode *kernelChannels = builder.CreatePHI(builder.nativeInt(), 1);
        PHINode *kernelColumns  = builder.CreatePHI(builder.nativeInt(), 1);
        PHINode *kernelRows     = builder.CreatePHI(builder.nativeInt(), 1);
        PHINode *kernelFrames   = builder.CreatePHI(builder.nativeInt(), 1);
        Value *kernelSize = builder.CreateMul(builder.CreateMul(builder.CreateMul(kernelChannels, kernelColumns), kernelRows), kernelFrames);
        likely_expression dst(newExpression::createCall(builder, dstType, builder.CreateMul(dstChannels, results), dstColumns, dstRows, dstFrames, builder.nullData()), dimensionsType);
        builder.undefineAll(args, false);

        // Load scalar values
        BasicBlock *scalarMatrixPromotion = BasicBlock::Create(builder.getContext(), "scalar_matrix_promotion", builder.GetInsertBlock()->getParent());
        builder.CreateBr(scalarMatrixPromotion);
        builder.SetInsertPoint(scalarMatrixPromotion);
        vector<likely_const_expr> thunkSrcs = srcs;
        vector<unique_ptr<const likely_expression>> scalars;
        for (likely_const_expr &thunkSrc : thunkSrcs)
            if (!likely_multi_dimension(*thunkSrc) && isMat(thunkSrc->value->getType())) {
                thunkSrc = new likely_expression(builder.CreateLoad(builder.CreateGEP(builder.data(thunkSrc), builder.zero())), *thunkSrc);
                scalars.push_back(unique_ptr<const likely_expression>(thunkSrc));
            }

        // Finally, do the computation
        BasicBlock *computation = BasicBlock::Create(builder.getContext(), "computation", builder.GetInsertBlock()->getParent());
        builder.CreateBr(computation);
        builder.SetInsertPoint(computation);

        Metadata metadata;
        if      (likely_heterogeneous(builder.env->type)) metadata = generateHeterogeneous(builder, args, thunkSrcs, dst, kernelSize);
        else if (likely_parallel(builder.env->type))      metadata = generateParallel     (builder, args, thunkSrcs, dst, kernelSize);
        else                                              metadata = generateSerial       (builder, args, thunkSrcs, dst, kernelSize);

        results->addIncoming(builder.constant(metadata.results), entry);
        dstType->addIncoming(builder.typeType(dst), entry);
        kernelChannels->addIncoming(metadata.collapsedAxis.find("c") != metadata.collapsedAxis.end() ? dstChannels : builder.one().value, entry);
        kernelColumns->addIncoming (metadata.collapsedAxis.find("x") != metadata.collapsedAxis.end() ? dstColumns  : builder.one().value, entry);
        kernelRows->addIncoming    (metadata.collapsedAxis.find("y") != metadata.collapsedAxis.end() ? dstRows     : builder.one().value, entry);
        kernelFrames->addIncoming  (metadata.collapsedAxis.find("t") != metadata.collapsedAxis.end() ? dstFrames   : builder.one().value, entry);
        return new likely_expression(dst);
    }

    Metadata generateSerial(Builder &builder, likely_const_ast args, const vector<likely_const_expr> &srcs, likely_expression &dst, Value *kernelSize) const
    {
        return generateCommon(builder, args, srcs, dst, builder.zero(), kernelSize);
    }

    Metadata generateParallel(Builder &builder, likely_const_ast args, const vector<likely_const_expr> &srcs, likely_expression &dst, Value *kernelSize) const
    {
        BasicBlock *entry = builder.GetInsertBlock();

        vector<Type*> parameterTypes;
        for (const likely_const_expr src : srcs)
            parameterTypes.push_back(src->value->getType());
        parameterTypes.push_back(dst.value->getType());
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
            for (size_t i=0; i<srcs.size()+1; i++) {
                const likely_type &type = i < srcs.size() ? srcs[i]->type : dst.type;
                thunkSrcs.push_back(new likely_expression(builder.CreateLoad(builder.CreateStructGEP(parameterStruct, unsigned(i))), type));
            }
            likely_expr kernelDst = const_cast<likely_expr>(thunkSrcs.back()); thunkSrcs.pop_back();
            kernelDst->value->setName("dst");

            metadata = generateCommon(builder, args, thunkSrcs, *kernelDst, start, stop);
            for (likely_const_expr thunkSrc : thunkSrcs)
                delete thunkSrc;

            dst.type = *kernelDst;
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
        }

        Value *parameterStruct = builder.CreateAlloca(parameterStructType);
        for (size_t i=0; i<srcs.size()+1; i++) {
            const likely_expression &src = i < srcs.size() ? *srcs[i] : dst;
            builder.CreateStore(src, builder.CreateStructGEP(parameterStruct, unsigned(i)));
        }

        builder.CreateCall3(likelyFork, builder.module()->getFunction(thunk->getName()), parameterStruct, kernelSize);
        return metadata;
    }

    Metadata generateHeterogeneous(Builder &, likely_const_ast, const vector<likely_const_expr> &, likely_expression &, Value *) const
    {
        assert(!"Not implemented");
        return Metadata();
    }

    Metadata generateCommon(Builder &builder, likely_const_ast args, const vector<likely_const_expr> &srcs, likely_expression &dst, Value *start, Value *stop) const
    {
        Metadata metadata;
        BasicBlock *entry = builder.GetInsertBlock();
        BasicBlock *steps = BasicBlock::Create(builder.getContext(), "steps", entry->getParent());
        builder.CreateBr(steps);
        builder.SetInsertPoint(steps);
        PHINode *channelStep;
        channelStep = builder.CreatePHI(builder.nativeInt(), 1); // Defined after we know the number of results
        Value *columnStep, *rowStep, *frameStep;
        builder.steps(&dst, channelStep, &columnStep, &rowStep, &frameStep);

        kernelAxis *axis = NULL;
        for (int axis_index=0; axis_index<4; axis_index++) {
            string name;
            bool multiElement;
            Value *elements, *step;

            switch (axis_index) {
              case 0:
                name = "t";
                multiElement = likely_multi_frame(dst);
                elements = builder.frames(&dst);
                step = frameStep;
                break;
              case 1:
                name = "y";
                multiElement = likely_multi_row(dst);
                elements = builder.rows(&dst);
                step = rowStep;
                break;
              case 2:
                name = "x";
                multiElement = likely_multi_column(dst);
                elements = builder.columns(&dst);
                step = columnStep;
                break;
              default:
                name = "c";
                multiElement = likely_multi_channel(dst);
                elements = builder.channels(&dst);
                step = channelStep;
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

        if (args->type == likely_ast_list) {
            for (size_t j=0; j<args->num_atoms; j++)
                builder.define(args->atoms[j]->atom, new kernelArgument(*srcs[j], dst, channelStep, axis->node));
        } else {
            builder.define(args->atom, new kernelArgument(*srcs[0], dst, channelStep, axis->node));
        }

        unique_ptr<const likely_expression> result(builder.expression(ast->atoms[2]));

        const vector<likely_const_expr> expressions = result->subexpressionsOrSelf();
        for (likely_const_expr e : expressions)
            dst.type = likely_type_from_types(dst, *e);

        metadata.results = expressions.size();
        channelStep->addIncoming(builder.constant(metadata.results), entry);
        for (size_t i=0; i<metadata.results; i++) {
            StoreInst *store = builder.CreateStore(builder.cast(expressions[i], dst), builder.CreateGEP(builder.data(&dst), builder.CreateAdd(axis->offset, builder.constant(i))));
            store->setMetadata("llvm.mem.parallel_loop_access", axis->node);
        }

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

    static bool getPairs(likely_const_ast ast, vector<pair<likely_const_ast,likely_const_ast>> &pairs)
    {
        pairs.clear();
        if (!ast) return true;
        if (ast->type != likely_ast_list) return false;
        if (ast->num_atoms == 0) return true;

        if (ast->atoms[0]->type == likely_ast_list) {
            for (size_t i=0; i<ast->num_atoms; i++) {
                if (ast->atoms[i]->num_atoms != 2) {
                    pairs.clear();
                    return false;
                }
                pairs.push_back(pair<likely_const_ast,likely_const_ast>(ast->atoms[i]->atoms[0], ast->atoms[i]->atoms[1]));
            }
        } else {
            if (ast->num_atoms != 2) {
                pairs.clear();
                return false;
            }
            pairs.push_back(pair<likely_const_ast,likely_const_ast>(ast->atoms[0], ast->atoms[1]));
        }
        return true;
    }

    static Value *getDimensions(Builder &builder, const vector<pair<likely_const_ast,likely_const_ast>> &pairs, const char *axis, const vector<likely_const_expr> &srcs, likely_type *type)
    {
        Value *result = NULL;
        for (const auto &pair : pairs) // Look for a dimensionality expression
            if (!strcmp(axis, pair.first->atom)) {
                result = builder.cast(unique_ptr<const likely_expression>(builder.expression(pair.second)).get(), likely_matrix_native);
                break;
            }

        // Use default dimensionality
        if (result == NULL) {
            if (srcs.empty()) {
                result = builder.constant(1);
            } else {
                if      (!strcmp(axis, "channels")) result = builder.channels(srcs[0]);
                else if (!strcmp(axis, "columns"))  result = builder.columns (srcs[0]);
                else if (!strcmp(axis, "rows"))     result = builder.rows    (srcs[0]);
                else                                result = builder.frames  (srcs[0]);
            }
        }

        const bool multiElement = !llvm::isa<Constant>(result) || (llvm::cast<Constant>(result)->getUniqueInteger().getZExtValue() > 1);
        if      (!strcmp(axis, "channels")) likely_set_multi_channel(type, multiElement);
        else if (!strcmp(axis, "columns"))  likely_set_multi_column (type, multiElement);
        else if (!strcmp(axis, "rows"))     likely_set_multi_row    (type, multiElement);
        else                                likely_set_multi_frame  (type, multiElement);
        return result;
    }
};

class kernelExpression : public Operator
{
    size_t minParameters() const { return 2; }
    size_t maxParameters() const { return 3; }
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const { return new Kernel(builder.env, ast); }
};
LIKELY_REGISTER_EXPRESSION(kernel, "=>")

struct Definition : public ScopedExpression
{
    Definition(likely_env env, likely_const_ast ast)
        : ScopedExpression(env, ast) {}

private:
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_env env = this->env;
        swap(builder.env, env);
        unique_ptr<const likely_expression> op(builder.expression(this->ast));
        swap(builder.env, env);

        return op.get() ? op->evaluate(builder, ast) : NULL;
    }

    size_t minParameters() const { return 0; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }
};

struct EvaluatedExpression : public ScopedExpression
{
    // Requries that `parent` stays valid through the lifetime of this class.
    // We avoid retaining `parent` to avoid a circular dependency.
    EvaluatedExpression(likely_env parent, likely_const_ast ast)
        : ScopedExpression(likely_new_env(parent), likely_retain_ast(ast))
    {
        likely_release_env(env->parent);
        likely_set_abandoned(&env->type, true);
        likely_set_offline(&env->type, false);
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
        return likely_retain_env(reinterpret_cast<const EvaluatedExpression*>(expr)->get());
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

        likely_const_mat m = get()->result;
        if (likely_elements(m) == 1) {
            // Promote to scalar
            return new likely_expression(builder.constant(likely_element(m, 0, 0, 0, 0), m->type));
        } else {
            // Return the matrix
            return new likely_expression(ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(builder.getContext(), 8*sizeof(m)), uintptr_t(m)), builder.toLLVM(m->type)), m->type);
        }
    }

    size_t minParameters() const { return 0; }
    size_t maxParameters() const { return 4; }
};

struct Variable : public likely_expression
{
    Variable(Builder &builder, likely_const_expr expr, const string &name)
    {
        type = *expr;
        value = builder.CreateAlloca(expr->value->getType(), 0, name);
        set(builder, expr);
    }

    void set(Builder &builder, likely_const_expr expr) const
    {
        builder.CreateStore(builder.cast(expr, type), value);
    }

    static const Variable *dynamicCast(likely_const_expr expr)
    {
        return (expr && (expr->uid() == UID())) ? static_cast<const Variable*>(expr) : NULL;
    }

private:
    static int UID() { return __LINE__; }
    int uid() const { return UID(); }

    likely_const_expr evaluate(Builder &builder, likely_const_ast) const
    {
        return new likely_expression(builder.CreateLoad(value), type);
    }
};

class defineExpression : public Operator
{
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast lhs = ast->atoms[1];
        likely_const_ast rhs = ast->atoms[2];
        const char *name = (lhs->type == likely_ast_list) ? lhs->atoms[0]->atom : lhs->atom;
        likely_env env = builder.env;

        if (likely_global(env->type)) {
            assert(!env->value);
            if (lhs->type == likely_ast_list) {
                // Export symbol
                vector<likely_type> types;
                for (size_t i=1; i<lhs->num_atoms; i++) {
                    if (lhs->atoms[i]->type == likely_ast_list)
                        return error(lhs->atoms[i], "expected an atom name parameter type");
                    types.push_back(likely_type_from_string(lhs->atoms[i]->atom));
                }

                if (likely_offline(env->type)) {
                    TRY_EXPR(builder, rhs, expr);
                    const Lambda *lambda = static_cast<const Lambda*>(expr.get());
                    env->value = lambda->generate(builder, types, name, false);
                } else {
                    JITFunction *function = new JITFunction(name, rhs, env, types, true, false, false);
                    if (function->function) {
                        sys::DynamicLibrary::AddSymbol(name, function->function);
                        env->value = function;
                    } else {
                        delete function;
                    }
                }
            } else {
                if (lambdaExpression::isLambda(rhs)) {
                    // Global variable
                    env->value = new Definition(env, rhs);
                } else {
                    // Global value
                    env->value = new EvaluatedExpression(env, rhs);
                }
            }

            likely_set_erratum(&env->type, !env->value);
            return NULL;
        } else {
            likely_const_expr expr = builder.expression(rhs);
            if (expr) {
                const Variable *variable = Variable::dynamicCast(builder.lookup(name));
                if (variable) variable->set(builder, expr);
                else          builder.define(name, new Variable(builder, expr, name));
            }
            return expr;
        }
    }
};
LIKELY_REGISTER_EXPRESSION(define, "=")

class importExpression : public Operator
{
    size_t maxParameters() const { return 1; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast file = ast->atoms[1];
        if (file->type == likely_ast_list)
            return error(file, "expected a file name");

        const string fileName = string(file->atom) + ".l";
        ifstream stream(fileName);
        const string source((istreambuf_iterator<char>(stream)),
                             istreambuf_iterator<char>());
        if (source.empty())
            return error(file, "unable to open file");

        likely_env parent = builder.env;
        likely_ast source_ast = likely_ast_from_string(source.c_str(), true);
        builder.env = likely_repl(source_ast, parent, NULL, NULL);
        likely_release_ast(source_ast);
        likely_release_env(parent);
        if (likely_erratum(builder.env->type)) return NULL;
        else                                   return new likely_expression();
    }
};
LIKELY_REGISTER(import)

JITFunction::JITFunction(const string &name, likely_const_ast ast, likely_const_env parent, const vector<likely_type> &parameters, bool abandon, bool interpreter, bool arrayCC)
    : env(likely_new_env(parent)), parameters(parameters)
{
    function = NULL;
    ref_count = 1;

    if (abandon) {
        likely_release_env(env->parent);
        likely_set_abandoned(&env->type, true);
    }

// No libffi support for Windows
#ifdef _WIN32
    interpreter = false;
#endif // _WIN32

    if (!lambdaExpression::isLambda(ast)) {
        likely_expression::error(ast, "expected a lambda expression");
        return;
    }

    env->module = new likely_module();
    likely_set_base(&env->type, true);
    Builder builder(env);
    unique_ptr<const likely_expression> result(builder.expression(ast));
    unique_ptr<const Symbol> expr(static_cast<const Lambda*>(result.get())->generate(builder, parameters, name, arrayCC));
    value = expr ? expr->value : NULL;
    type = expr ? expr->type : likely_type(likely_matrix_void);
    if (!value)
        return;

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
    }
//    env->module->module->dump();
}

#ifdef LIKELY_IO
#include "likely/io.h"

class printExpression : public Operator
{
    size_t minParameters() const { return 1; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }
    void *symbol() const { return (void*) likely_print_va; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        Function *likelyPrint = builder.module()->getFunction("likely_print_va");
        if (!likelyPrint) {
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), builder.multiDimension(), true);
            likelyPrint = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_print_va", builder.module());
            likelyPrint->setCallingConv(CallingConv::C);
            likelyPrint->setDoesNotAlias(0);
            likelyPrint->setDoesNotAlias(1);
            likelyPrint->setDoesNotCapture(1);
            sys::DynamicLibrary::AddSymbol("lle_X_likely_print_va", (void*) lle_X_likely_print_va);
        }

        vector<Value*> rawArgs;
        for (size_t i=1; i<ast->num_atoms; i++) {
            TRY_EXPR(builder, ast->atoms[i], arg);
            rawArgs.push_back(*arg);
        }
        rawArgs.push_back(builder.nullMat());

        vector<Value*> matArgs;
        for (Value *rawArg : rawArgs)
            if (rawArg->getType() == builder.multiDimension()) {
                matArgs.push_back(rawArg);
            } else {
                // Intermediate matricies will be released when this function returns
                matArgs.push_back(stringExpression::createCall(builder, rawArg));
            }

        Value *result = builder.CreateCall(likelyPrint, matArgs);

        return new likely_expression(result, likely_matrix_i8);
    }

    static GenericValue lle_X_likely_print_va(FunctionType *, const vector<GenericValue> &Args)
    {
        vector<likely_const_mat> mv;
        for (size_t i=0; i<Args.size()-1; i++)
            mv.push_back((likely_const_mat) Args[i].PointerVal);
        return GenericValue(likely_print_n(mv.data(), mv.size()));
    }
};
LIKELY_REGISTER(print)

class readExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_read; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        Function *likelyRead = builder.module()->getFunction("likely_read");
        if (!likelyRead) {
            Type *params[] = { Type::getInt8PtrTy(builder.getContext()), builder.nativeInt() };
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), params, false);
            likelyRead = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_read", builder.module());
            likelyRead->setCallingConv(CallingConv::C);
            likelyRead->setDoesNotAlias(0);
            likelyRead->setDoesNotAlias(1);
            likelyRead->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall2(likelyRead, *arg, builder.constant(likely_file_binary)), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(read)

class writeExpression : public SimpleBinaryOperator
{
    void *symbol() const { return (void*) likely_write; }

    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &arg1, const unique_ptr<const likely_expression> &arg2) const
    {
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
        }
        return new likely_expression(builder.CreateCall2(likelyWrite, *arg1, *arg2), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(write)

class decodeExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_decode; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        Function *likelyDecode = builder.module()->getFunction("likely_decode");
        if (!likelyDecode) {
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), builder.multiDimension(), false);
            likelyDecode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_decode", builder.module());
            likelyDecode->setCallingConv(CallingConv::C);
            likelyDecode->setDoesNotAlias(0);
            likelyDecode->setDoesNotAlias(1);
            likelyDecode->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyDecode, builder.CreatePointerCast(*arg, builder.multiDimension())), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(decode)

class encodeExpression : public SimpleBinaryOperator
{
    void *symbol() const { return (void*) likely_encode; }

    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &arg1, const unique_ptr<const likely_expression> &arg2) const
    {
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
        }
        return new likely_expression(builder.CreateCall2(likelyEncode, builder.CreatePointerCast(*arg1, builder.multiDimension()), *arg2), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(encode)

class renderExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_render; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        Function *likelyRender = builder.module()->getFunction("likely_render");
        if (!likelyRender) {
            Type *params[] = { builder.multiDimension(), Type::getInt8PtrTy(builder.getContext()) };
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), params, false);
            likelyRender = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_render", builder.module());
            likelyRender->setCallingConv(CallingConv::C);
            likelyRender->setDoesNotAlias(0);
            likelyRender->setDoesNotAlias(1);
            likelyRender->setDoesNotCapture(1);
            likelyRender->setDoesNotAlias(2);
            likelyRender->setDoesNotCapture(2);
            likelyRender->setDoesNotAlias(3);
            likelyRender->setDoesNotCapture(3);
        }
        return new likely_expression(builder.CreateCall3(likelyRender, *arg, ConstantPointerNull::get(Type::getDoublePtrTy(builder.getContext())), ConstantPointerNull::get(Type::getDoublePtrTy(builder.getContext()))), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(render)

class showExpression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_decode; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        Function *likelyShow = builder.module()->getFunction("likely_show");
        if (!likelyShow) {
            Type *params[] = { builder.multiDimension(), Type::getInt8PtrTy(builder.getContext()) };
            FunctionType *functionType = FunctionType::get(builder.multiDimension(), params, false);
            likelyShow = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_show", builder.module());
            likelyShow->setCallingConv(CallingConv::C);
            likelyShow->setDoesNotAlias(1);
            likelyShow->setDoesNotCapture(1);
            likelyShow->setDoesNotAlias(2);
            likelyShow->setDoesNotCapture(2);
        }
        return new likely_expression(builder.CreateCall2(likelyShow, *arg, ConstantPointerNull::get(Type::getInt8PtrTy(builder.getContext()))), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(show)
#endif // LIKELY_IO

class md5Expression : public SimpleUnaryOperator
{
    void *symbol() const { return (void*) likely_md5; }

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
        }
        return new likely_expression(builder.CreateCall(likelyMd5, *arg), likely_matrix_multi_dimension);
    }
};
LIKELY_REGISTER(md5)

} // namespace (anonymous)

likely_env likely_new_env(likely_const_env parent)
{
    likely_env env = (likely_env) malloc(sizeof(likely_environment));
    env->type = likely_environment_void;
    if (parent) {
        likely_set_offline(&env->type, likely_offline(parent->type));
        likely_set_execution(&env->type, likely_execution(parent->type));
    }
    env->parent = likely_retain_env(parent);
    env->ast = NULL;
    env->module = parent ? parent->module : NULL;
    env->value = NULL;
    env->result = NULL;
    env->ref_count = 1;
    env->num_children = 0;
    env->children = NULL;
    return env;
}

likely_env likely_new_env_jit()
{
    return likely_new_env(RootEnvironment::get());
}

likely_env likely_new_env_offline(const char *file_name)
{
    likely_env env = likely_new_env(RootEnvironment::get());
    env->module = new OfflineModule(file_name);
    likely_set_offline(&env->type, true);
    likely_set_base(&env->type, true);
    return env;
}

likely_env likely_retain_env(likely_const_env env)
{
    if (env) const_cast<likely_env>(env)->ref_count++;
    return const_cast<likely_env>(env);
}

static mutex ChildLock; // For modifying likely_environment::children

void likely_release_env(likely_const_env env)
{
    if (!env) return;
    assert(env->ref_count > 0);
    if (--const_cast<likely_env>(env)->ref_count) return;

    { // Remove ourself as our parent's child
        lock_guard<mutex> guard(ChildLock);
        const likely_env parent = const_cast<likely_env>(env->parent);
        for (size_t i=0; i<parent->num_children; i++)
            if (env == parent->children[i]) {
                parent->children[i] = parent->children[--parent->num_children];
                break;
            }
    }

    // Do this early to guarantee the environment for these lifetime of these classes
    if (likely_definition(env->type)) delete env->value;
    else                              likely_release(env->result);

    likely_release_ast(env->ast);
    free(env->children);
    if (likely_base(env->type))
        delete env->module;
    if (!likely_abandoned(env->type))
        likely_release_env(env->parent);

    free(const_cast<likely_env>(env));
}

bool likely_offline(likely_environment_type type) { return likely_get_bool(type, likely_environment_offline); }
void likely_set_offline(likely_environment_type *type, bool offline) { likely_set_bool(type, offline, likely_environment_offline); }
bool likely_parallel(likely_environment_type type) { return likely_get_bool(type, likely_environment_parallel); }
void likely_set_parallel(likely_environment_type *type, bool parallel) { likely_set_bool(type, parallel, likely_environment_parallel); }
bool likely_heterogeneous(likely_environment_type type) { return likely_get_bool(type, likely_environment_heterogeneous); }
void likely_set_heterogeneous(likely_environment_type *type, bool heterogeneous) { likely_set_bool(type, heterogeneous, likely_environment_heterogeneous); }
likely_environment_type likely_execution(likely_environment_type type) { return likely_get(type, likely_environment_execution); }
void likely_set_execution(likely_environment_type *type, likely_environment_type execution) { likely_set(type, execution, likely_environment_execution); }
bool likely_erratum(likely_environment_type type) { return likely_get_bool(type, likely_environment_erratum); }
void likely_set_erratum(likely_environment_type *type, bool erratum) { likely_set_bool(type, erratum, likely_environment_erratum); }
bool likely_definition(likely_environment_type type) { return likely_get_bool(type, likely_environment_definition); }
void likely_set_definition(likely_environment_type *type, bool definition) { likely_set_bool(type, definition, likely_environment_definition); }
bool likely_global(likely_environment_type type) { return likely_get_bool(type, likely_environment_global); }
void likely_set_global(likely_environment_type *type, bool global) { likely_set_bool(type, global, likely_environment_global); }
bool likely_abandoned(likely_environment_type type) { return likely_get_bool(type, likely_environment_abandoned); }
void likely_set_abandoned(likely_environment_type *type, bool abandoned) { likely_set_bool(type, abandoned, likely_environment_abandoned); }
bool likely_base(likely_environment_type type) { return likely_get_bool(type, likely_environment_base); }
void likely_set_base(likely_environment_type *type, bool base) { likely_set_bool(type, base, likely_environment_base); }

likely_mat likely_dynamic(likely_vtable vtable, likely_const_mat *mv)
{
    void *function = NULL;
    for (size_t i=0; i<vtable->functions.size(); i++) {
        const unique_ptr<JITFunction> &jitFunction = vtable->functions[i];
        for (size_t j=0; j<vtable->n; j++)
            if (mv[j]->type != jitFunction->parameters[j])
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
            types.push_back(mv[i]->type);
        vtable->functions.push_back(unique_ptr<JITFunction>(new JITFunction("likely_vtable_entry", vtable->ast, vtable->env, types, true, false, true)));
        function = vtable->functions.back()->function;
        if (function == NULL)
            return NULL;
    }

    return reinterpret_cast<likely_function_n>(function)(mv);
}

likely_fun likely_compile(likely_const_ast ast, likely_const_env env, likely_type type, ...)
{
    if (!ast || !env) return NULL;
    vector<likely_type> types;
    va_list ap;
    va_start(ap, type);
    while (type != likely_matrix_void) {
        types.push_back(type);
        type = va_arg(ap, likely_type);
    }
    va_end(ap);
    return static_cast<likely_fun>(new JITFunction("likely_jit_function", ast, env, types, false, false, false));
}

likely_fun likely_retain_function(likely_const_fun f)
{
    if (f) const_cast<likely_fun>(f)->ref_count++;
    return const_cast<likely_fun>(f);
}

void likely_release_function(likely_const_fun f)
{
    if (!f) return;
    assert(f->ref_count > 0);
    if (--const_cast<likely_fun>(f)->ref_count) return;
    delete static_cast<const JITFunction*>(f);
}

likely_env likely_eval(likely_ast ast, likely_env parent)
{
    if (!ast || !parent)
        return NULL;

    { // Check against parent environment for precomputed result
        lock_guard<mutex> guard(ChildLock);
        for (size_t i=0; i<parent->num_children; i++)
            if (!likely_ast_compare(ast, parent->children[i]->ast))
                return likely_retain_env(parent->children[i]);
    }

    likely_env env = likely_new_env(parent);
    likely_set_definition(&env->type, (ast->type == likely_ast_list) && (ast->num_atoms > 0) && !strcmp(ast->atoms[0]->atom, "="));
    likely_set_global(&env->type, true);
    env->ast = likely_retain_ast(ast);

    { // We reallocate space for more children when our power-of-two sized buffer is full
        lock_guard<mutex> guard(ChildLock);
        if ((parent->num_children == 0) || !(parent->num_children & (parent->num_children - 1)))
            parent->children = (likely_const_env*) realloc(parent->children, (parent->num_children == 0 ? 1 : 2 * parent->num_children) * sizeof(likely_const_env));
        parent->children[parent->num_children++] = env;
    }

    if (likely_definition(env->type)) {
        Builder(env).expression(ast); // Returns NULL
    } else if (likely_offline(env->type)) {
        // Do nothing, evaluating expressions in an offline environment is a no-op.
    } else {
        likely_const_ast lambda = likely_ast_from_string("(-> () (scalar <ast>))", false);
        likely_release_ast(lambda->atoms[0]->atoms[2]->atoms[1]); // <ast>
        const_cast<likely_ast&>(lambda->atoms[0]->atoms[2]->atoms[1]) = likely_retain_ast(ast); // Copy because we will modify ast->type

        JITFunction jit("likely_jit_function", lambda->atoms[0], env, vector<likely_type>(), false, true, false);
        if (jit.EE) {
            env->result = (likely_mat) jit.EE->runFunction(cast<Function>(jit.value), vector<GenericValue>()).PointerVal;
        } else if (jit.function) {
            // Fallback is a compiled function if the interpreter isn't supported
            env->result = reinterpret_cast<likely_function_0>(jit.function)();
        } else {
            likely_set_erratum(&env->type, true);
        }

        likely_release_ast(lambda);
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
            repl_callback(env, context ? context : (void*)(!likely_definition(env->type) && env->result && (likely_elements(env->result) > 0)));
        if (likely_erratum(env->type))
            break;
    }

    return env;
}

likely_const_env likely_evaluated_expression(likely_const_expr expr)
{
    return EvaluatedExpression::get(expr);
}

likely_mat likely_md5(likely_const_mat buffer)
{
    MD5 md5;
    md5.update(ArrayRef<uint8_t>(reinterpret_cast<const uint8_t*>(buffer->data), likely_bytes(buffer)));
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
