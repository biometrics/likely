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
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/ExecutionEngine/ObjectCache.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Vectorize.h>
#include <cstdarg>
#include <functional>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>

#include "likely/backend.h"

using namespace llvm;
using namespace std;

namespace {

static IntegerType *NativeInt = NULL;
static LLVMContext &C = getGlobalContext();

struct MatType
{
    static MatType MultiDimension;

    MatType() : llvm(NULL), likely(likely_matrix_void) {}
    MatType(Type *llvm, likely_type likely)
        : llvm(llvm), likely(likely) {}

    operator Type*() const { return llvm; }
    operator ArrayRef<Type*>() const { return ArrayRef<Type*>((Type**)&llvm, 1); }
    operator likely_type() const { return likely; }

    static MatType get(likely_type likely)
    {
        auto result = likelyLUT.find(likely);
        if (result != likelyLUT.end())
            return result->second;

        Type *llvm;
        if (!likely_multi_dimension(likely) && likely_depth(likely)) {
            llvm = scalar(likely);
        } else {
            likely_mat str = likely_type_to_string(likely);
            const string name = (const char*) str->data;
            llvm = PointerType::getUnqual(StructType::create(name,
                                                             NativeInt, // bytes
                                                             NativeInt, // ref_count
                                                             NativeInt, // channels
                                                             NativeInt, // columns
                                                             NativeInt, // rows
                                                             NativeInt, // frames
                                                             NativeInt, // type
                                                             ArrayType::get(Type::getInt8Ty(C), 0), // data
                                                             NULL));
            likely_release(str);
        }

        MatType t(llvm, likely);
        likelyLUT[t.likely] = t;
        llvmLUT[t.llvm] = t;
        return t;
    }

    static MatType get(Type *llvm)
    {
        auto result = llvmLUT.find(llvm);
        likely_assert(result != llvmLUT.end(), "invalid pointer for type lookup");
        return result->second;
    }

    static Type *scalar(likely_type type, bool pointer = false)
    {
        const size_t bits = likely_depth(type);
        const bool floating = likely_floating(type);
        if (floating) {
            if      (bits == 16) return pointer ? Type::getHalfPtrTy(C)   : Type::getHalfTy(C);
            else if (bits == 32) return pointer ? Type::getFloatPtrTy(C)  : Type::getFloatTy(C);
            else if (bits == 64) return pointer ? Type::getDoublePtrTy(C) : Type::getDoubleTy(C);
        } else {
            if      (bits == 1)  return pointer ? Type::getInt1PtrTy(C)  : (Type*)Type::getInt1Ty(C);
            else if (bits == 8)  return pointer ? Type::getInt8PtrTy(C)  : (Type*)Type::getInt8Ty(C);
            else if (bits == 16) return pointer ? Type::getInt16PtrTy(C) : (Type*)Type::getInt16Ty(C);
            else if (bits == 32) return pointer ? Type::getInt32PtrTy(C) : (Type*)Type::getInt32Ty(C);
            else if (bits == 64) return pointer ? Type::getInt64PtrTy(C) : (Type*)Type::getInt64Ty(C);
        }
        likely_assert(false, "ty invalid matrix bits: %d and floating: %d", bits, floating);
        return NULL;
    }

private:
    static map<likely_type, MatType> likelyLUT;
    static map<Type*, MatType> llvmLUT;
    Type *llvm;
    likely_type likely;
};
MatType MatType::MultiDimension;
map<likely_type, MatType> MatType::likelyLUT;
map<Type*, MatType> MatType::llvmLUT;

struct Builder;

} // namespace (anonymous)

struct likely_expression
{
    Value *value;
    likely_type type;

    likely_expression(Value *value = NULL, likely_type type = likely_matrix_void)
        : value(value), type(type) {}

    virtual ~likely_expression()
    {
        for (const likely_expression *e : expressions_)
            delete e;
    }

    virtual likely_expression *evaluate(Builder &builder, likely_const_ast ast) const;

    virtual int rightHandAtoms() const { return 1; }

    operator Value*() const { return value; }
    operator likely_type() const { return type; }

    Value *take()
    {
        Value *result = value;
        delete this; // With great power comes great responsibility
        return result;
    }

    const likely_expression *takeExpression(size_t index)
    {
        const likely_expression *result = NULL;
        if (index < expressions_.size())
            swap(expressions_[index], result);
        return result;
    }

    void append(const likely_expression *e) { expressions_.push_back(e); }

    vector<const likely_expression*> expressions() const
    {
        if (expressions_.empty()) {
            vector<const likely_expression*> expressions;
            expressions.push_back(this);
            return expressions;
        } else {
            return expressions_;
        }
    }

    static likely_expression *error(likely_const_ast ast, const char *message)
    {
        likely_throw(ast, message);
        return NULL;
    }

    // Idiom to ensure that specified library symbols aren't stripped when optimizing executable size
    virtual void *symbol() const { return NULL; }

    static likely_expression constant(uint64_t value, likely_type type = likely_matrix_native)
    {
        const unsigned depth = unsigned(likely_depth(type));
        return likely_expression(Constant::getIntegerValue(Type::getIntNTy(C, depth), APInt(depth, value)), type);
    }

    static likely_expression constant(double value, likely_type type)
    {
        const size_t depth = likely_depth(type);
        if (likely_floating(type)) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return likely_expression(ConstantFP::get(Type::getDoubleTy(C), value), type);
            else if (depth == 32) return likely_expression(ConstantFP::get(Type::getFloatTy(C), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return likely_expression(NULL, likely_matrix_void); }
        } else {
            return constant(uint64_t(value), type);
        }
    }

    static likely_expression zero(likely_type type = likely_matrix_native) { return constant((uint64_t) 0, type); }
    static likely_expression one (likely_type type = likely_matrix_native) { return constant((uint64_t) 1, type); }
    static likely_expression intMax(likely_type type) { const size_t bits = likely_depth(type); return constant((uint64_t) (1 << (bits - (likely_signed(type) ? 1 : 0)))-1, bits); }
    static likely_expression intMin(likely_type type) { const size_t bits = likely_depth(type); return constant((uint64_t) (likely_signed(type) ? (1 << (bits - 1)) : 0), bits); }
    static likely_expression typeType(likely_type type) { return constant((uint64_t) type, likely_matrix_type_type); }
    static likely_expression nullMat() { return likely_expression(ConstantPointerNull::get(cast<PointerType>((Type*)MatType::MultiDimension)), likely_matrix_void); }
    static likely_expression nullData() { return likely_expression(ConstantPointerNull::get(Type::getInt8PtrTy(C)), likely_matrix_native); }

    static likely_type validFloatType(likely_type type)
    {
        likely_set_floating(&type, true);
        likely_set_signed(&type, true);
        likely_set_depth(&type, likely_depth(type) > 32 ? 64 : 32);
        return type;
    }

    static const likely_expression *lookup(likely_const_env env, const char *name)
    {
        if (!env) return NULL;
        if (likely_definition(env->type) && env->name && !strcmp(env->name, name)) return env->value;
        return lookup(env->parent, name);
    }

    static likely_resources *lookupResources(likely_const_env env)
    {
        if (!env) return NULL;
        if (env->resources) return env->resources;
        return lookupResources(env->parent);
    }

    static void define(likely_env &env, const char *name, const likely_expression *value)
    {
        env = likely_new_env(env);
        likely_release_env(env->parent);
        likely_set_definition(&env->type, true);
        env->name = new char[strlen(name)+1];
        strcpy((char*) env->name, name);
        env->value = value;
    }

    static const likely_expression *undefine(likely_env &env, const char *name)
    {
        assert(likely_definition(env->type));
        likely_assert(!strcmp(env->name, name), "undefine variable mismatch");
        const likely_expression *value = env->value;
        likely_env old = env;
        env = likely_retain_env(env->parent);
        likely_release_env(old);
        return value;
    }

private:
    vector<const likely_expression*> expressions_;
};

namespace {

struct UniqueAST : public unique_ptr<const likely_abstract_syntax_tree, function<void(likely_const_ast)>>
{
    UniqueAST(likely_const_ast ast = NULL)
        : unique_ptr<const likely_abstract_syntax_tree, function<void(likely_const_ast)>>(ast, likely_release_ast) {}
};

struct UniqueASTL : public vector<likely_const_ast>
{
    ~UniqueASTL()
    {
        for (likely_const_ast ast : *this)
            likely_release_ast(ast);
    }

    UniqueAST ast() const
    {
        for (likely_const_ast ast : *this)
            likely_retain_ast(ast);
        return likely_new_list(data(), size());
    }

    void retain(likely_const_ast ast) { push_back(likely_retain_ast(ast)); }
};

#define TRY_EXPR(BUILDER, AST, EXPR)                                 \
const unique_ptr<likely_expression> EXPR((BUILDER).expression(AST)); \
if (!EXPR.get()) return NULL;                                        \

} // namespace (anonymous)

struct likely_resources
{
    Module *module;
    vector<likely_expression*> expressions;

    likely_resources(bool native)
    {
        if (!NativeInt) {
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

            NativeInt = Type::getIntNTy(C, unsigned(likely_depth(likely_matrix_native)));
            MatType::MultiDimension = MatType::get(likely_matrix_multi_dimension);
        }

        module = new Module("likely_module", C);
        likely_assert(module != NULL, "failed to create module");
        if (native) module->setTargetTriple(sys::getProcessTriple());
    }

    static TargetMachine *getTargetMachine(bool JIT)
    {
        static const string processTriple = sys::getProcessTriple();
        static const Target *TheTarget = NULL;
        if (TheTarget == NULL) {
            string error;
            TheTarget = TargetRegistry::lookupTarget(processTriple, error);
            likely_assert(TheTarget != NULL, "target lookup failed with error: %s", error.c_str());
        }

        TargetOptions TO;
        TO.LessPreciseFPMADOption = true;
        TO.UnsafeFPMath = true;
        TO.NoInfsFPMath = true;
        TO.NoNaNsFPMath = true;
        TO.AllowFPOpFusion = FPOpFusion::Fast;

        string targetTriple = sys::getProcessTriple();
#ifdef _WIN32
        if (JIT) targetTriple += "-elf";
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

    virtual ~likely_resources()
    {
        for (likely_expression *e : expressions)
            delete e;
        delete module;
    }
};

namespace {

class JITFunctionCache : public ObjectCache
{
    map<hash_code, unique_ptr<MemoryBuffer>> cache;
    const Module *currentModule = NULL;

    void notifyObjectCompiled(const Module *M, const MemoryBuffer *Obj)
    {
        if (M == currentModule)
            cache[currentHash].reset(MemoryBuffer::getMemBufferCopy(Obj->getBuffer()));
    }

    MemoryBuffer *getObject(const Module *M)
    {
        if (M == currentModule)
            if (MemoryBuffer *buffer = cache[currentHash].get())
                return MemoryBuffer::getMemBufferCopy(buffer->getBuffer());
        return NULL;
    }

public:
    hash_code currentHash = 0;

    bool alert(const Module *M)
    {
        string str;
        raw_string_ostream ostream(str);
        M->print(ostream, NULL);
        ostream.flush();
        currentModule = M;
        currentHash = hash_value(str);
        return cache[currentHash].get();
    }
};
static JITFunctionCache TheJITFunctionCache;

class OfflineResources : public likely_resources
{
    const string fileName;

public:
    OfflineResources(const string &fileName, bool native)
        : likely_resources(native), fileName(fileName) {}

    ~OfflineResources()
    {
        const string extension = fileName.substr(fileName.find_last_of(".") + 1);

        string errorInfo;
        tool_output_file output(fileName.c_str(), errorInfo, sys::fs::F_None);
        if (extension == "ll") {
            module->print(output.os(), NULL);
        } else if (extension == "bc") {
            WriteBitcodeToFile(module, output.os());
        } else {
            PassManager pm;
            formatted_raw_ostream fos(output.os());
            static TargetMachine *TM = getTargetMachine(false);
            TM->addPassesToEmitFile(pm, fos, extension == "s" ? TargetMachine::CGFT_AssemblyFile : TargetMachine::CGFT_ObjectFile);
            pm.run(*module);
        }

        likely_assert(errorInfo.empty(), "failed to write to: %s with error: %s", fileName.c_str(), errorInfo.c_str());
        output.keep();
    }
};

struct Builder : public IRBuilder<>
{
    likely_env env;
    map<string, shared_ptr<likely_expression>> locals;

    Builder(likely_env env)
        : IRBuilder<>(C), env(likely_retain_env(env)) {}

    ~Builder() { likely_release_env(env); }

    likely_expression channels(const likely_expression *matrix) { return likely_multi_channel(*matrix) ? likely_expression(CreateLoad(CreateStructGEP(*matrix, 2), "channels"), likely_matrix_native) : likely_expression::one(); }
    likely_expression columns (const likely_expression *matrix) { return likely_multi_column (*matrix) ? likely_expression(CreateLoad(CreateStructGEP(*matrix, 3), "columns" ), likely_matrix_native) : likely_expression::one(); }
    likely_expression rows    (const likely_expression *matrix) { return likely_multi_row    (*matrix) ? likely_expression(CreateLoad(CreateStructGEP(*matrix, 4), "rows"    ), likely_matrix_native) : likely_expression::one(); }
    likely_expression frames  (const likely_expression *matrix) { return likely_multi_frame  (*matrix) ? likely_expression(CreateLoad(CreateStructGEP(*matrix, 5), "frames"  ), likely_matrix_native) : likely_expression::one(); }
    likely_expression data    (const likely_expression *matrix) { return likely_expression(CreatePointerCast(CreateStructGEP(*matrix, 7), MatType::scalar(*matrix, true)), likely_data(*matrix)); }

    void steps(const likely_expression *matrix, Value *channelStep, Value **columnStep, Value **rowStep, Value **frameStep)
    {
        *columnStep = CreateMul(channels(matrix), channelStep, "x_step");
        *rowStep    = CreateMul(columns(matrix), *columnStep, "y_step");
        *frameStep  = CreateMul(rows(matrix), *rowStep, "t_step");
    }

    likely_expression cast(const likely_expression *x, likely_type type)
    {
        if (likely_data(*x) == likely_data(type))
            return likely_expression(*x, type);
        if (likely_depth(type) == 0) {
            likely_set_depth(&type, likely_depth(*x));
            if (likely_floating(type))
                type = likely_expression::validFloatType(type);
        }
        Type *dstType = MatType::scalar(type);
        return likely_expression(CreateCast(CastInst::getCastOpcode(*x, likely_signed(*x), dstType, likely_signed(type)), *x, dstType), type);
    }

    const likely_expression *lookup(const char *name) const { return likely_expression::lookup(env, name); }
    likely_resources *lookupResources() const { return likely_expression::lookupResources(env); }
    void   define(const char *name, const likely_expression *e) { likely_expression::define(env, name, e); }
    const likely_expression *undefine(const char *name)         { return likely_expression::undefine(env, name); }

    void undefineAll(likely_const_ast args, bool deleteExpression)
    {
        if (args->is_list) {
            for (size_t i=0; i<args->num_atoms; i++) {
                const likely_expression *expression = undefine(args->atoms[args->num_atoms-i-1]->atom);
                if (deleteExpression) delete expression;
            }
        } else {
            const likely_expression *expression = undefine(args->atom);
            if (deleteExpression) delete expression;
        }
    }

    Module *module() { return lookupResources()->module; }

    likely_expression *expression(likely_const_ast ast);
};

struct Symbol : public likely_expression
{
    Symbol(Value *value = NULL, likely_type type = likely_matrix_void)
        : likely_expression(value, type) {}

    likely_expression *evaluate(Builder &builder, likely_const_ast ast) const
    {
        Function *definition = cast<Function>(value);
        Function *symbol = cast<Function>(builder.module()->getOrInsertFunction(definition->getName(), definition->getFunctionType()));
        if (symbol->arg_size() != (ast->is_list ? ast->num_atoms-1 : 0))
            return error(ast, "incorrect argument count");

        vector<Value*> args;
        if (ast->is_list) {
            Function::arg_iterator it = symbol->arg_begin();
            for (size_t i=1; i<ast->num_atoms; i++, it++) {
                TRY_EXPR(builder, ast->atoms[i], arg)
                args.push_back(builder.cast(arg.get(), MatType::get(it->getType())));
            }
        }

        return new likely_expression(builder.CreateCall(symbol, args), type);
    }
};

class JITFunction : public Symbol
{
    ExecutionEngine *EE = NULL;

public:
    likely_resources resources;
    likely_function function = NULL;
    hash_code hash = 0;
    const vector<likely_type> parameters;
    size_t ref_count = 1;

    JITFunction(const string &name, likely_const_ast ast, likely_env env, const vector<likely_type> &parameters, bool arrayCC);

    ~JITFunction()
    {
        resources.module = NULL;
        delete EE; // owns module
    }
};

class Operator : public likely_expression
{
    likely_expression *evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (!ast->is_list && (minParameters() > 0))
            return error(ast, "operator expected arguments");
        const size_t args = ast->is_list ? ast->num_atoms - 1 : 0;
        if ((args < minParameters()) || (args > maxParameters()))
            return errorArgc(ast, "operator", args, minParameters(), maxParameters());
        return evaluateOperator(builder, ast);
    }

    virtual likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const = 0;
    virtual size_t maxParameters() const = 0;
    virtual size_t minParameters() const { return maxParameters(); }

protected:
    static likely_expression *errorArgc(likely_const_ast ast, const string &function, size_t args, size_t minParams, size_t maxParams)
    {
        stringstream stream;
        stream << function << " with: " << minParams;
        if (maxParams != minParams)
            stream << "-" << maxParams;
        stream << " parameters passed: " << args << " arguments";
        return error(ast, stream.str().c_str());
    }
};

class VAOperator : public Operator
{
    size_t minParameters() const { return 2; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }
};

struct ScopedExpression : public Operator
{
    likely_env env;
    likely_const_ast ast;

    ScopedExpression(Builder &builder, likely_const_ast ast)
        : env(likely_retain_env(builder.env)), ast(likely_retain_ast(ast)) {}

    ~ScopedExpression()
    {
        likely_release_ast(ast);
        likely_release_env(env);
    }

private:
    size_t minParameters() const { return 0; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }
};

struct ScopedEnvironment
{
    Builder &builder;
    likely_env prev;

    ScopedEnvironment(Builder &builder, likely_env env = NULL, likely_resources *resources = NULL)
        : builder(builder), prev(builder.env)
    {
        builder.env = likely_new_env(env ? env : builder.env);
        if (resources) builder.env->resources = resources;
    }

    ~ScopedEnvironment()
    {
        builder.env->resources = NULL;
        likely_release_env(builder.env);
        builder.env = prev;
    }
};

} // namespace (anonymous)

likely_expression *likely_expression::evaluate(Builder &builder, likely_const_ast ast) const
{
    if (ast->is_list) {
        likely_expression *expression = new likely_expression();
        for (size_t i=0; i<ast->num_atoms; i++) {
            if (likely_expression *e = builder.expression(ast->atoms[i])) {
                expression->append(e);
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

    likely_virtual_table(Builder &builder, likely_const_ast ast, size_t n)
        : ScopedExpression(builder, ast), n(n) {}

    likely_expression *evaluateOperator(Builder &, likely_const_ast) const { return NULL; }
};

namespace {

static int getPrecedence(const char *op)
{
    if (!strcmp(op, "=" )) return 1;
    if (!strcmp(op, "->")) return 2;
    if (!strcmp(op, "=>")) return 2;
    if (!strcmp(op, "+>")) return 2;
    if (!strcmp(op, "#" )) return 3;
    if (!strcmp(op, "?" )) return 3;
    if (!strcmp(op, "<" )) return 4;
    if (!strcmp(op, "<=")) return 4;
    if (!strcmp(op, ">" )) return 4;
    if (!strcmp(op, ">=")) return 4;
    if (!strcmp(op, "==")) return 4;
    if (!strcmp(op, "!=")) return 4;
    if (!strcmp(op, "&" )) return 5;
    if (!strcmp(op, "^" )) return 5;
    if (!strcmp(op, "|" )) return 5;
    if (!strcmp(op, "<<")) return 5;
    if (!strcmp(op, "+" )) return 6;
    if (!strcmp(op, "-" )) return 6;
    if (!strcmp(op, "*" )) return 7;
    if (!strcmp(op, "/" )) return 7;
    if (!strcmp(op, "%" )) return 7;
    if (!strcmp(op, "??")) return 8;
    return 0;
}

static likely_env RootEnvironment = likely_new_env(NULL);

template <class E>
struct RegisterExpression
{
    RegisterExpression(const char *symbol)
    {
        likely_expression *e = new E();
        likely_expression::define(RootEnvironment, symbol, e);
        if (int precedence = getPrecedence(symbol))
            likely_insert_operator(symbol, precedence, e->rightHandAtoms());
    }
};
#define LIKELY_REGISTER_EXPRESSION(EXP, SYM) static struct RegisterExpression<EXP##Expression> Register##EXP##Expression(SYM);
#define LIKELY_REGISTER(EXP) LIKELY_REGISTER_EXPRESSION(EXP, #EXP)

class UnaryOperator : public Operator
{
    size_t maxParameters() const { return 1; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        return evaluateUnary(builder, ast->atoms[1]);
    }
    virtual likely_expression *evaluateUnary(Builder &builder, likely_const_ast arg) const = 0;
};

class SimpleUnaryOperator : public UnaryOperator
{
    likely_expression *evaluateUnary(Builder &builder, likely_const_ast arg) const
    {
        TRY_EXPR(builder, arg, expr)
        return evaluateSimpleUnary(builder, expr);
    }
    virtual likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const = 0;
};

struct MatrixType : public SimpleUnaryOperator
{
    likely_type t;
    MatrixType(likely_type t)
        : t(t)
    {
        value = typeType(t);
        type = likely_matrix_type_type;
    }

    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &x) const
    {
        return new likely_expression(builder.cast(x.get(), t));
    }
};

likely_expression *Builder::expression(likely_const_ast ast)
{
    if (ast->is_list) {
        if (ast->num_atoms == 0)
            return likely_expression::error(ast, "Empty expression");
        likely_const_ast op = ast->atoms[0];
        if (!op->is_list)
            if (const likely_expression *e = lookup(op->atom))
                return e->evaluate(*this, ast);
        TRY_EXPR(*this, op, e);
        return e->evaluate(*this, ast);
    }
    const string op = ast->atom;

    { // Is it a local variable?
        auto var = locals.find(op);
        if (var != locals.end())
            return var->second->evaluate(*this, ast);
    }

    if (const likely_expression *e = lookup(op.c_str()))
        return e->evaluate(*this, ast);

    if ((op.front() == '"') && (op.back() == '"'))
        return new likely_expression(CreateGlobalStringPtr(op.substr(1, op.length()-2)), likely_matrix_u8);

    { // Is it a number?
        char *p;
        const double value = strtod(op.c_str(), &p);
        if (*p == 0)
            return new likely_expression(likely_expression::constant(value, likely_type_from_value(value)));
    }

    { // Is it a type?
        bool ok;
        likely_type type = likely_type_field_from_string(op.c_str(), &ok);
        if (ok) return new MatrixType(type);
    }

    return likely_expression::error(ast, "unrecognized literal");
}

#define LIKELY_REGISTER_FIELD(FIELD)                                                                         \
class FIELD##Expression : public SimpleUnaryOperator                                                         \
{                                                                                                            \
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const \
    {                                                                                                        \
        return new likely_expression(builder.FIELD(arg.get()));                                              \
    }                                                                                                        \
};                                                                                                           \
LIKELY_REGISTER(FIELD)                                                                                       \

LIKELY_REGISTER_FIELD(channels)
LIKELY_REGISTER_FIELD(columns)
LIKELY_REGISTER_FIELD(rows)
LIKELY_REGISTER_FIELD(frames)

class notExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        return new likely_expression(builder.CreateXor(intMax(*arg), arg->value), *arg);
    }
};
LIKELY_REGISTER_EXPRESSION(not, "~")

class typeExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &, const unique_ptr<likely_expression> &arg) const
    {
        return new MatrixType(*arg);
    }
};
LIKELY_REGISTER(type)

class UnaryMathOperator : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &x) const
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
LIKELY_REGISTER_UNARY_MATH(fabs)
LIKELY_REGISTER_UNARY_MATH(floor)
LIKELY_REGISTER_UNARY_MATH(ceil)
LIKELY_REGISTER_UNARY_MATH(trunc)
LIKELY_REGISTER_UNARY_MATH(rint)
LIKELY_REGISTER_UNARY_MATH(nearbyint)
LIKELY_REGISTER_UNARY_MATH(round)

class SimpleBinaryOperator : public Operator
{
    size_t maxParameters() const { return 2; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[1], expr1)
        TRY_EXPR(builder, ast->atoms[2], expr2)
        return evaluateSimpleBinary(builder, expr1, expr2);
    }
    virtual likely_expression *evaluateSimpleBinary(Builder &builder, const unique_ptr<likely_expression> &arg1, const unique_ptr<likely_expression> &arg2) const = 0;
};

class ArithmeticOperator : public SimpleBinaryOperator
{
    likely_expression *evaluateSimpleBinary(Builder &builder, const unique_ptr<likely_expression> &lhs, const unique_ptr<likely_expression> &rhs) const
    {
        likely_type type = likely_type_from_types(*lhs, *rhs);
        return evaluateArithmetic(builder, builder.cast(lhs.get(), type), builder.cast(rhs.get(), type));
    }
    virtual likely_expression *evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const = 0;
};

class SimpleArithmeticOperator : public ArithmeticOperator
{
    likely_expression *evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
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
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, zero(lhs)), intMax(lhs), intMin(lhs)) : intMax(lhs).value;
                return builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0));
            } else {
                return builder.CreateAdd(lhs, rhs);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(add, "+")

class subtractExpression : public SimpleArithmeticOperator
{
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        if (likely_floating(lhs)) {
            return builder.CreateFSub(lhs, rhs);
        } else {
            if (likely_saturation(lhs)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), likely_signed(lhs) ? Intrinsic::ssub_with_overflow : Intrinsic::usub_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, zero(lhs)), intMax(lhs), intMin(lhs)) : intMin(lhs).value;
                return builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0));
            } else {
                return builder.CreateSub(lhs, rhs);
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
                Value *zero = likely_expression::zero(lhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateXor(builder.CreateICmpSGE(lhs, zero), builder.CreateICmpSGE(rhs, zero)), intMin(lhs), intMax(lhs)) : intMax(lhs).value;
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
                    Value *safe_i = builder.CreateAdd(n, builder.CreateZExt(builder.CreateICmpNE(builder.CreateOr(builder.CreateAdd(d, one(n)), builder.CreateAdd(n, intMin(n))), zero(n)), n.value->getType()));
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

class remExpression : public SimpleArithmeticOperator
{
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        return likely_floating(lhs) ? builder.CreateFRem(lhs, rhs)
                                    : (likely_signed(lhs) ? builder.CreateSRem(lhs, rhs)
                                                          : builder.CreateURem(lhs, rhs));
    }
};
LIKELY_REGISTER_EXPRESSION(rem, "%")

#define LIKELY_REGISTER_LOGIC(OP, SYM)                                                                                  \
class OP##Expression : public SimpleArithmeticOperator                                                                  \
{                                                                                                                       \
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const \
    {                                                                                                                   \
        return builder.Create##OP(lhs, rhs.value);                                                                      \
    }                                                                                                                   \
};                                                                                                                      \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                     \

LIKELY_REGISTER_LOGIC(And , "&"   )
LIKELY_REGISTER_LOGIC(Or  , "|"   )
LIKELY_REGISTER_LOGIC(Xor , "^"   )
LIKELY_REGISTER_LOGIC(Shl , "<<"  )
LIKELY_REGISTER_LOGIC(LShr, "lshr")
LIKELY_REGISTER_LOGIC(AShr, "ashr")

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                              \
class OP##Expression : public ArithmeticOperator                                                                                         \
{                                                                                                                                        \
    likely_expression *evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const            \
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

#define LIKELY_REGISTER_EQUALITY(OP, SYM)                                                                                     \
class OP##Expression : public ArithmeticOperator                                                                              \
{                                                                                                                             \
    likely_expression *evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const \
    {                                                                                                                         \
        return new likely_expression(likely_floating(lhs) ? builder.CreateFCmpO##OP(lhs, rhs)                                 \
                                                          : builder.CreateICmp##OP(lhs, rhs), likely_matrix_u1);              \
    }                                                                                                                         \
};                                                                                                                            \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                           \

LIKELY_REGISTER_EQUALITY(EQ, "==")
LIKELY_REGISTER_EQUALITY(NE, "!=")

class BinaryMathOperator : public SimpleBinaryOperator
{
    likely_expression *evaluateSimpleBinary(Builder &builder, const unique_ptr<likely_expression> &x, const unique_ptr<likely_expression> &n) const
    {
        const likely_type type = nIsInteger() ? x->type : likely_type_from_types(*x, *n);
        likely_expression xc(builder.cast(x.get(), validFloatType(type)));
        likely_expression nc(builder.cast(n.get(), nIsInteger() ? likely_type(likely_matrix_i32) : xc));
        return new likely_expression(builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), id(), xc.value->getType()), xc, nc), xc);
    }
    virtual Intrinsic::ID id() const = 0;
    virtual bool nIsInteger() const { return false; }
};

class powiExpression : public BinaryMathOperator
{
    Intrinsic::ID id() const { return Intrinsic::powi; }
    bool nIsInteger() const { return true; }
};
LIKELY_REGISTER(powi)

#define LIKELY_REGISTER_BINARY_MATH(OP)                \
class OP##Expression : public BinaryMathOperator       \
{                                                      \
    Intrinsic::ID id() const { return Intrinsic::OP; } \
};                                                     \
LIKELY_REGISTER(OP)                                    \

LIKELY_REGISTER_BINARY_MATH(pow)
LIKELY_REGISTER_BINARY_MATH(copysign)

struct Definition : public ScopedExpression
{
    Definition(Builder &builder, likely_const_ast ast)
        : ScopedExpression(builder, ast) {}

private:
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        unique_ptr<likely_expression> op;
        {
            ScopedEnvironment se(builder, env, builder.lookupResources());
            op.reset(builder.expression(this->ast));
        }
        return op.get() ? op->evaluate(builder, ast) : NULL;
    }
};

class Variable : public likely_expression
{
    likely_type types;
    AllocaInst *allocaInst;

public:
    Variable(Builder &builder, likely_expression *expr, const string &name)
    {
        types = *expr;
        allocaInst = builder.CreateAlloca(expr->value->getType(), 0, name);
        set(builder, expr);
    }

    void set(Builder &builder, likely_expression *expr)
    {
        builder.CreateStore(builder.cast(expr, types), allocaInst);
    }

private:
    likely_expression *evaluate(Builder &builder, likely_const_ast) const
    {
        return new likely_expression(builder.CreateLoad(allocaInst), types);
    }
};

class definedExpression : public Operator
{
    size_t maxParameters() const { return 2; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast name = ast->atoms[1];
        if (name->is_list)
            return error(name, "expected an atom");
        if (builder.lookup(name->atom)) return builder.expression(name);
        else                            return builder.expression(ast->atoms[2]);
    }
};
LIKELY_REGISTER_EXPRESSION(defined, "??")

class elementsExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        static FunctionType *functionType = FunctionType::get(NativeInt, MatType::MultiDimension, false);
        Function *likelyElements = builder.module()->getFunction("likely_elements");
        if (!likelyElements) {
            likelyElements = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_elements", builder.module());
            likelyElements->setCallingConv(CallingConv::C);
            likelyElements->setDoesNotAlias(1);
            likelyElements->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyElements, *arg), likely_matrix_native);
    }
    void *symbol() const { return (void*) likely_elements; }
};
LIKELY_REGISTER(elements)

class bytesExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        static FunctionType *functionType = FunctionType::get(NativeInt, MatType::MultiDimension, false);
        Function *likelyBytes = builder.module()->getFunction("likely_bytes");
        if (!likelyBytes) {
            likelyBytes = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_bytes", builder.module());
            likelyBytes->setCallingConv(CallingConv::C);
            likelyBytes->setDoesNotAlias(1);
            likelyBytes->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyBytes, *arg), likely_matrix_native);
    }
    void *symbol() const { return (void*) likely_bytes; }
};
LIKELY_REGISTER(bytes)

class newExpression : public Operator
{
    size_t maxParameters() const { return 6; }
    size_t minParameters() const { return 0; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const size_t n = ast->num_atoms - 1;
        unique_ptr<likely_expression> type;
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
            case 6: type.reset(new likely_expression(typeType(validFloatType(likely_matrix_native))));
            case 5: channels = one();
            case 4: columns  = one();
            case 3: rows     = one();
            case 2: frames   = one();
            case 1: data     = nullData();
            default:           break;
        }

        return new likely_expression(createCall(builder, *type, channels, columns, rows, frames, data), MatType::MultiDimension);
    }

    void *symbol() const { return (void*) likely_new; }

public:
    static CallInst *createCall(Builder &builder, Value *type, Value *channels, Value *columns, Value *rows, Value *frames, Value *data)
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type* params[] = { NativeInt, NativeInt, NativeInt, NativeInt, NativeInt, Type::getInt8PtrTy(C) };
            functionType = FunctionType::get(MatType::MultiDimension, params, false);
        }

        Function *likelyNew = builder.module()->getFunction("likely_new");
        if (!likelyNew) {
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
    likely_expression *evaluateUnary(Builder &builder, likely_const_ast arg) const
    {
        likely_expression *argExpr = builder.expression(arg);
        if (!argExpr)
            return NULL;

        if (argExpr->value && (argExpr->value->getType() == MatType::MultiDimension))
            return argExpr;

        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type* params[] = { NativeInt, Type::getDoubleTy(C) };
            functionType = FunctionType::get(MatType::MultiDimension, params, true);
        }

        Function *likelyScalars = builder.module()->getFunction("likely_scalars");
        if (!likelyScalars) {
            likelyScalars = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_scalars", builder.module());
            likelyScalars->setCallingConv(CallingConv::C);
            likelyScalars->setDoesNotAlias(0);
        }

        vector<Value*> args;
        likely_type type = likely_matrix_void;
        for (const likely_expression *e : argExpr->expressions()) {
            args.push_back(builder.cast(e, likely_matrix_f64));
            type = likely_type_from_types(type, e->type);
        }
        args.push_back(ConstantFP::get(C, APFloat::getNaN(APFloat::IEEEdouble)));
        args.insert(args.begin(), typeType(type));

        likely_expression result(builder.CreateCall(likelyScalars, args), *argExpr);
        delete argExpr;
        return new likely_expression(result);
    }

    void *symbol() const { return (void*) likely_scalars; }
};
LIKELY_REGISTER(scalar)

class stringExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), likely_matrix_i8);
    }

    void *symbol() const { return (void*) likely_string; }

public:
    static CallInst *createCall(Builder &builder, Value *string)
    {
        static FunctionType *functionType = FunctionType::get(MatType::MultiDimension, Type::getInt8PtrTy(C), false);
        Function *likelyString = builder.module()->getFunction("likely_string");
        if (!likelyString) {
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
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), *arg);
    }

    void *symbol() const { return (void*) likely_copy; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(MatType::MultiDimension, MatType::MultiDimension, false);
        Function *likelyCopy = builder.module()->getFunction("likely_copy");
        if (!likelyCopy) {
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
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), *arg);
    }

    void *symbol() const { return (void*) likely_retain; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(MatType::MultiDimension, MatType::MultiDimension, false);
        Function *likelyRetain = builder.module()->getFunction("likely_retain");
        if (!likelyRetain) {
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
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), *arg);
    }

    void *symbol() const { return (void*) likely_release; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(Type::getVoidTy(C), MatType::MultiDimension, false);
        Function *likelyRelease = builder.module()->getFunction("likely_release");
        if (!likelyRelease) {
            likelyRelease = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_release", builder.module());
            likelyRelease->setCallingConv(CallingConv::C);
            likelyRelease->setDoesNotAlias(1);
            likelyRelease->setDoesNotCapture(1);
        }
        return builder.CreateCall(likelyRelease, m);
    }
};
LIKELY_REGISTER(release)

struct Lambda : public ScopedExpression
{
    Lambda(Builder &builder, likely_const_ast ast)
        : ScopedExpression(builder, ast) {}

    Symbol *generate(Builder &builder, vector<likely_type> types, string name, bool arrayCC) const
    {
        size_t n;
        if (ast->is_list && (ast->num_atoms > 1))
            if (ast->atoms[1]->is_list) n = ast->atoms[1]->num_atoms;
            else                        n = 1;
        else                            n = 0;

        while (types.size() < n)
            types.push_back(MatType::MultiDimension);

        vector<Type*> llvmTypes;
        if (arrayCC) {
            // Array calling convention
            llvmTypes.push_back(PointerType::get(MatType::MultiDimension, 0));
        } else {
            for (const likely_type &t : types)
                llvmTypes.push_back(MatType::get(t));
        }

        BasicBlock *originalInsertBlock = builder.GetInsertBlock();
        Function *tmpFunction = cast<Function>(builder.module()->getOrInsertFunction(name+"_tmp", FunctionType::get(Type::getVoidTy(C), llvmTypes, false)));
        BasicBlock *entry = BasicBlock::Create(C, "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        vector<likely_expression> arguments;
        if (arrayCC) {
            Value *argumentArray = tmpFunction->arg_begin();
            for (size_t i=0; i<types.size(); i++)
                arguments.push_back(likely_expression(builder.CreateLoad(builder.CreateGEP(argumentArray, constant(i))), types[i]));
        } else {
            Function::arg_iterator it = tmpFunction->arg_begin();
            size_t i = 0;
            while (it != tmpFunction->arg_end())
                arguments.push_back(likely_expression(it++, types[i++]));
        }

        for (size_t i=0; i<arguments.size(); i++) {
            stringstream name; name << "arg_" << i;
            arguments[i].value->setName(name.str());
        }

        likely_expression *result = evaluateFunction(builder, arguments);
        if (!result) return NULL;

        builder.CreateRet(*result);
        const likely_type return_type = result->type;

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
        return new Symbol(function, return_type);
    }

private:
    void *symbol() const { return (void*) likely_dynamic; }

    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const size_t parameters = this->ast->atoms[1]->is_list ? this->ast->atoms[1]->num_atoms : 1;
        const size_t arguments = ast->is_list ? ast->num_atoms - 1 : 0;
        if (parameters != arguments)
            return errorArgc(ast, "lambda", arguments, parameters, parameters);

        vector<likely_expression> args;
        for (size_t i=0; i<arguments; i++) {
            TRY_EXPR(builder, ast->atoms[i+1], arg)
            args.push_back(likely_expression(arg->value, arg->type));
        }

        ScopedEnvironment se(builder, env, builder.lookupResources());
        return evaluateFunction(builder, args);
    }

    likely_expression *evaluateFunction(Builder &builder, const vector<likely_expression> &args) const
    {
        // Do dynamic dispatch if the type isn't fully specified
        bool dynamic = false;
        for (const likely_expression &arg : args)
            dynamic = dynamic || (arg.type == MatType::MultiDimension);
        dynamic = dynamic || (args.size() < ast->atoms[1]->num_atoms);

        if (dynamic) {
            likely_vtable vtable = new likely_virtual_table(builder, ast, args.size());
            builder.lookupResources()->expressions.push_back(vtable);

            static PointerType *vTableType = PointerType::getUnqual(StructType::create(C, "VTable"));
            static FunctionType *likelyDynamicType = NULL;
            if (likelyDynamicType == NULL) {
                Type* params[] = { vTableType, PointerType::get(MatType::MultiDimension, 0) };
                likelyDynamicType = FunctionType::get(MatType::MultiDimension, params, false);
            }

            Function *likelyDynamic = builder.module()->getFunction("likely_dynamic");
            if (!likelyDynamic) {
                likelyDynamic = Function::Create(likelyDynamicType, GlobalValue::ExternalLinkage, "likely_dynamic", builder.module());
                likelyDynamic->setCallingConv(CallingConv::C);
                likelyDynamic->setDoesNotAlias(0);
                likelyDynamic->setDoesNotAlias(1);
                likelyDynamic->setDoesNotCapture(1);
                likelyDynamic->setDoesNotAlias(2);
                likelyDynamic->setDoesNotCapture(2);
            }

            Value *matricies = builder.CreateAlloca(MatType::MultiDimension, constant(args.size()));
            for (size_t i=0; i<args.size(); i++)
                builder.CreateStore(args[i], builder.CreateGEP(matricies, constant(i)));
            Value* args[] = { ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(C, 8*sizeof(vtable)), uintptr_t(vtable)), vTableType), matricies };
            return new likely_expression(builder.CreateCall(likelyDynamic, args), MatType::MultiDimension);
        }

        return evaluateLambda(builder, args);
    }

    virtual likely_expression *evaluateLambda(Builder &builder, const vector<likely_expression> &args) const
    {
        if (ast->atoms[1]->is_list) {
            for (size_t i=0; i<args.size(); i++)
                builder.define(ast->atoms[1]->atoms[i]->atom, new likely_expression(args[i]));
        } else {
            builder.define(ast->atoms[1]->atom, new likely_expression(args[0]));
        }
        likely_expression *result = builder.expression(ast->atoms[2]);
        builder.undefineAll(ast->atoms[1], true);
        return result;
    }
};

class lambdaExpression : public Operator
{
    size_t maxParameters() const { return 2; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        return new Lambda(builder, ast);
    }
};
LIKELY_REGISTER_EXPRESSION(lambda, "->")

class letExpression : public Operator
{
    size_t maxParameters() const { return 2; }

    static bool addDefinition(likely_const_ast def, UniqueASTL &names, UniqueASTL &values)
    {
        if (!def->is_list || (def->num_atoms != 2))
            return likely_throw(def, "expected a tuple");

        if (def->atoms[0]->is_list)
            return likely_throw(def->atoms[0], "expected an atom");

        names.retain(def->atoms[0]);
        values.retain(def->atoms[1]);
        return true;
    }

    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast defs = ast->atoms[1];
        if (!defs->is_list || (defs->num_atoms == 0))
            return error(ast->atoms[1], "expected a definition list");

        UniqueASTL names, values;
        values.push_back(NULL); // The lambda function will be placed here
        if (defs->atoms[0]->is_list) {
            for (size_t i=0; i<defs->num_atoms; i++)
                if (!addDefinition(defs->atoms[i], names, values))
                    return NULL;
        } else {
            if (!addDefinition(defs, names, values))
                return NULL;
        }

        UniqueASTL lambda;
        lambda.push_back(likely_new_atom("->"));
        lambda.retain(names.ast().get());
        lambda.retain(ast->atoms[2]);
        values[0] = likely_retain_ast(lambda.ast().get());
        return builder.expression(values.ast().get());
    }
};
LIKELY_REGISTER(let)

class beginExpression : public VAOperator
{
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        ScopedEnvironment se(builder);
        for (size_t i=1; i<ast->num_atoms-1; i++) {
            TRY_EXPR(builder, ast->atoms[i], expr);
        }
        return builder.expression(ast->atoms[ast->num_atoms-1]);
    }
};
LIKELY_REGISTER_EXPRESSION(begin, "{")

class indexExpression : public VAOperator
{
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[1], expr)
        likely_expression *result = new likely_expression();
        for (size_t i=2; i<ast->num_atoms; i++) {
            const size_t index = atoi(ast->atoms[i]->atom);
            const likely_expression *e = expr->takeExpression(index);
            if (e) {
                result->append(e);
            } else {
                delete result;
                return error(ast->atoms[i], "index out of range");
            }
        }
        return result;
    }
};
LIKELY_REGISTER_EXPRESSION(index, "[")

struct Label : public likely_expression
{
    Label(BasicBlock *basicBlock) : likely_expression(basicBlock) {}

private:
    likely_expression *evaluate(Builder &builder, likely_const_ast) const
    {
        BasicBlock *basicBlock = cast<BasicBlock>(value);
        builder.CreateBr(basicBlock);
        return new Label(basicBlock);
    }
};

class labelExpression : public Operator
{
    size_t maxParameters() const { return 1; }
    int rightHandAtoms() const { return 0; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const string name = ast->atoms[1]->atom;
        BasicBlock *label = BasicBlock::Create(C, name, builder.GetInsertBlock()->getParent());
        builder.CreateBr(label);
        builder.SetInsertPoint(label);
        builder.locals.insert(pair<string,shared_ptr<likely_expression>>(name, shared_ptr<likely_expression>(new Label(label))));
        return new Label(label);
    }
};
LIKELY_REGISTER_EXPRESSION(label, "#")

class ifExpression : public Operator
{
    size_t minParameters() const { return 2; }
    size_t maxParameters() const { return 3; }
    int rightHandAtoms() const { return 1; }

    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        Function *function = builder.GetInsertBlock()->getParent();
        const bool hasElse = ast->num_atoms == 4;

        TRY_EXPR(builder, ast->atoms[1], Cond)
        BasicBlock *True = BasicBlock::Create(C, "then", function);
        BasicBlock *False = hasElse ? BasicBlock::Create(C, "else", function) : NULL;
        BasicBlock *End = BasicBlock::Create(C, "end", function);
        builder.CreateCondBr(*Cond, True, hasElse ? False : End);

        builder.SetInsertPoint(True);
        TRY_EXPR(builder, ast->atoms[2], t)
        if (True->empty() || !True->back().isTerminator())
            builder.CreateBr(End);

        if (hasElse) {
            builder.SetInsertPoint(False);
            TRY_EXPR(builder, ast->atoms[3], f)
            if (False->empty() || !False->back().isTerminator())
                builder.CreateBr(End);

            builder.SetInsertPoint(End);
            PHINode *phi = builder.CreatePHI(t->value->getType(), 2);
            phi->addIncoming(*t, True);
            if (hasElse) phi->addIncoming(*f, False);
            return new likely_expression(phi, *t);
        } else {
            builder.SetInsertPoint(End);
            return new likely_expression(NULL, likely_matrix_void);
        }
    }
};
LIKELY_REGISTER_EXPRESSION(if, "?")

struct Kernel : public Lambda
{
    Kernel(Builder &builder, likely_const_ast ast)
        : Lambda(builder, ast) {}

protected:
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
            loop = BasicBlock::Create(C, "loop_" + name, entry->getParent());
            builder.CreateBr(loop);
            builder.SetInsertPoint(loop);
            value = builder.CreatePHI(NativeInt, 2, name);
            cast<PHINode>(value)->addIncoming(start, entry);
            type = likely_matrix_native;
        }

        virtual void close(Builder &builder)
        {
            Value *increment = builder.CreateAdd(value, one(), name + "_increment");
            exit = BasicBlock::Create(C, name + "_exit", loop->getParent());
            latch = builder.CreateCondBr(builder.CreateICmpEQ(increment, stop, name + "_test"), exit, loop);
            cast<PHINode>(value)->addIncoming(increment, builder.GetInsertBlock());
            builder.SetInsertPoint(exit);
        }
    };

    struct Metadata
    {
        set<string> collapsedAxis;
        size_t results;
    };

    virtual Metadata generateCommon(Builder &builder, likely_const_ast args, const vector<likely_expression> &srcs, likely_expression &dst, Value *start, Value *stop) const
    {
        Metadata metadata;
        BasicBlock *entry = builder.GetInsertBlock();
        BasicBlock *steps = BasicBlock::Create(C, "steps", entry->getParent());
        builder.CreateBr(steps);
        builder.SetInsertPoint(steps);
        PHINode *channelStep;
        channelStep = builder.CreatePHI(NativeInt, 1); // Defined after we know the number of results
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
                else       axis = new kernelAxis(builder, name, zero(), elements, step, axis);
                builder.define(name.c_str(), axis); // takes ownership of axis
            } else {
                builder.define(name.c_str(), new likely_expression(zero(), likely_matrix_native));
            }
        }
        builder.define("i", new likely_expression(axis->offset, likely_matrix_native));

        if (args->is_list) {
            for (size_t j=0; j<args->num_atoms; j++)
                builder.define(args->atoms[j]->atom, new kernelArgument(srcs[j], dst, channelStep, axis->node));
        } else {
            builder.define(args->atom, new kernelArgument(srcs[0], dst, channelStep, axis->node));
        }

        unique_ptr<likely_expression> result(builder.expression(ast->atoms[2]));
        const vector<const likely_expression*> expressions = result->expressions();
        for (const likely_expression *e : expressions)
            dst.type = likely_type_from_types(dst, *e);

        metadata.results = expressions.size();
        channelStep->addIncoming(constant(metadata.results), entry);
        for (size_t i=0; i<metadata.results; i++) {
            StoreInst *store = builder.CreateStore(builder.cast(expressions[i], dst), builder.CreateGEP(builder.data(&dst), builder.CreateAdd(axis->offset, constant(i))));
            store->setMetadata("llvm.mem.parallel_loop_access", axis->node);
        }

        axis->close(builder);
        metadata.collapsedAxis = axis->tryCollapse();

        builder.undefineAll(args, true);
        delete builder.undefine("i");
        delete builder.undefine("c");
        delete builder.undefine("x");
        delete builder.undefine("y");
        delete builder.undefine("t");
        return metadata;
    }

private:
    class kernelArgument : public Operator
    {
        likely_expression matrix;
        likely_type kernel;
        Value *channelStep;
        MDNode *node;

    public:
        kernelArgument(const likely_expression &matrix, likely_type kernel, Value *channelStep, MDNode *node)
            : matrix(matrix), kernel(kernel), channelStep(channelStep), node(node) {}

    private:
        size_t maxParameters() const { return 0; }
        likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
        {
            if (ast->is_list)
                return error(ast, "kernel operator does not take arguments");

            if (!isa<PointerType>(matrix.value->getType()))
                return new likely_expression(matrix);

            Value *i;
            if (((matrix ^ kernel) & likely_matrix_multi_dimension) == 0) {
                // This matrix has the same dimensionality as the kernel
                i = *builder.lookup("i");
            } else {
                Value *columnStep, *rowStep, *frameStep;
                builder.steps(&matrix, channelStep, &columnStep, &rowStep, &frameStep);
                i = zero();
                if (likely_multi_channel(matrix)) i = builder.CreateMul(*builder.lookup("c"), channelStep);
                if (likely_multi_column (matrix)) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("x"), columnStep), i);
                if (likely_multi_row    (matrix)) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("y"), rowStep   ), i);
                if (likely_multi_frame  (matrix)) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("t"), frameStep ), i);
            }
            LoadInst *load = builder.CreateLoad(builder.CreateGEP(builder.data(&matrix), i));

            load->setMetadata("llvm.mem.parallel_loop_access", node);
            return new likely_expression(load, matrix);
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
                MDNode *tmp = MDNode::getTemporary(C, metadata);
                metadata.push_back(tmp);
                node = MDNode::get(C, metadata);
                tmp->replaceAllUsesWith(node);
                MDNode::deleteTemporary(tmp);
            }

            if (parent)
                parent->child = this;
            offset = builder.CreateAdd(parent ? parent->offset : zero().value, builder.CreateMul(step, value), name + "_offset");
        }

        void close(Builder &builder)
        {
            Loop::close(builder);
            latch->setMetadata("llvm.loop", node);
            if (parent) parent->close(builder);
        }

        bool referenced() const { return value->getNumUses() > 2; }

        set<string> tryCollapse()
        {
            if (parent)
                return parent->tryCollapse();

            set<string> collapsedAxis;
            collapsedAxis.insert(name);
            if (referenced())
                return collapsedAxis;

            while (child && !child->referenced()) {
                // Collapse the child loop into us
                child->offset->replaceAllUsesWith(value);
                child->latch->setCondition(ConstantInt::getTrue(C));
                DeleteDeadPHIs(child->loop);
                MergeBlockIntoPredecessor(child->loop);
                MergeBlockIntoPredecessor(child->exit);
                collapsedAxis.insert(child->name);
                child = child->child;
            }
            return collapsedAxis;
        }
    };

    void *symbol() const { return (void*) likely_fork; }

    likely_expression *evaluateLambda(Builder &builder, const vector<likely_expression> &srcs) const
    {
        likely_type kernelType = likely_matrix_void;
        if (!srcs.empty())
            likely_set_execution(&kernelType, likely_execution(srcs.front()));

        vector<pair<likely_const_ast,likely_const_ast>> pairs;
        if (ast->num_atoms == 4)
            getPairs(ast->atoms[3], pairs);

        for (const auto &pair : pairs)
            if (!strcmp("type", pair.first->atom) && !pair.second->is_list)
                kernelType |= likely_type_field_from_string(pair.second->atom, NULL);

        const likely_const_ast args = ast->atoms[1];
        assert(srcs.size() == args->is_list ? args->num_atoms : 1);
        if (args->is_list) {
            for (size_t j=0; j<args->num_atoms; j++)
                builder.define(args->atoms[j]->atom, &srcs[j]);
        } else {
            builder.define(args->atom, &srcs[0]);
        }

        BasicBlock *entry = builder.GetInsertBlock();
        Value *dstChannels = getDimensions(builder, pairs, "channels", srcs, &kernelType);
        Value *dstColumns  = getDimensions(builder, pairs, "columns" , srcs, &kernelType);
        Value *dstRows     = getDimensions(builder, pairs, "rows"    , srcs, &kernelType);
        Value *dstFrames   = getDimensions(builder, pairs, "frames"  , srcs, &kernelType);

        BasicBlock *allocation = BasicBlock::Create(C, "allocation", builder.GetInsertBlock()->getParent());
        builder.CreateBr(allocation);
        builder.SetInsertPoint(allocation);
        PHINode *results   = builder.CreatePHI(NativeInt, 1);
        PHINode *dstType   = builder.CreatePHI(NativeInt, 1);
        PHINode *kernelChannels = builder.CreatePHI(NativeInt, 1);
        PHINode *kernelColumns  = builder.CreatePHI(NativeInt, 1);
        PHINode *kernelRows     = builder.CreatePHI(NativeInt, 1);
        PHINode *kernelFrames   = builder.CreatePHI(NativeInt, 1);
        Value *kernelSize = builder.CreateMul(builder.CreateMul(builder.CreateMul(kernelChannels, kernelColumns), kernelRows), kernelFrames);
        likely_expression dst(newExpression::createCall(builder, dstType, builder.CreateMul(dstChannels, results), dstColumns, dstRows, dstFrames, nullData()), kernelType);
        builder.undefineAll(args, false);

        Metadata metadata;
        if      (likely_heterogeneous(kernelType)) metadata = generateHeterogeneous(builder, args, srcs, dst, kernelSize);
        else if (likely_parallel(kernelType))      metadata = generateParallel     (builder, args, srcs, dst, kernelSize);
        else                                       metadata = generateSerial       (builder, args, srcs, dst, kernelSize);

        results->addIncoming(constant(metadata.results), entry);
        dstType->addIncoming(typeType(dst), entry);
        kernelChannels->addIncoming(metadata.collapsedAxis.find("c") != metadata.collapsedAxis.end() ? dstChannels : one().value, entry);
        kernelColumns->addIncoming (metadata.collapsedAxis.find("x") != metadata.collapsedAxis.end() ? dstColumns  : one().value, entry);
        kernelRows->addIncoming    (metadata.collapsedAxis.find("y") != metadata.collapsedAxis.end() ? dstRows     : one().value, entry);
        kernelFrames->addIncoming  (metadata.collapsedAxis.find("t") != metadata.collapsedAxis.end() ? dstFrames   : one().value, entry);
        return new likely_expression(dst);
    }

    Metadata generateSerial(Builder &builder, likely_const_ast args, const vector<likely_expression> &srcs, likely_expression &dst, Value *kernelSize) const
    {
        return generateCommon(builder, args, srcs, dst, zero(), kernelSize);
    }

    Metadata generateParallel(Builder &builder, likely_const_ast args, const vector<likely_expression> &srcs, likely_expression &dst, Value *kernelSize) const
    {
        BasicBlock *allocation = builder.GetInsertBlock();

        Function *thunk;
        Metadata metadata;
        {
            static FunctionType *thunkType = NULL;
            if (!thunkType) {
                Type* params[] = { PointerType::get(MatType::MultiDimension, 0), NativeInt, NativeInt };
                thunkType = FunctionType::get(Type::getVoidTy(C), params, false);
            }

            thunk = ::cast<Function>(builder.module()->getOrInsertFunction(builder.GetInsertBlock()->getParent()->getName().str() + "_thunk", thunkType));
            thunk->addFnAttr(Attribute::NoUnwind);
            thunk->setCallingConv(CallingConv::C);
            thunk->setLinkage(GlobalValue::PrivateLinkage);
            thunk->setDoesNotAlias(1);
            thunk->setDoesNotCapture(1);

            Function::arg_iterator it = thunk->arg_begin();
            Value *thunkMatrixArray = it++;
            Value *start = it++;
            Value *stop = it++;

            builder.SetInsertPoint(BasicBlock::Create(C, "entry", thunk));
            vector<likely_expression> thunkSrcs;
            for (size_t i=0; i<srcs.size()+1; i++) {
                const likely_type type = i < srcs.size() ? srcs[i].type : dst.type;
                Type *llvmType = MatType::get(type);
                if (!likely_multi_dimension(type))
                    llvmType = llvmType->getPointerTo();
                Value *val = builder.CreatePointerCast(builder.CreateLoad(builder.CreateGEP(thunkMatrixArray, constant(i))), llvmType);
                if (!likely_multi_dimension(type))
                    val = builder.CreateLoad(val);
                thunkSrcs.push_back(likely_expression(val, type));
            }
            likely_expression kernelDst = thunkSrcs.back(); thunkSrcs.pop_back();
            kernelDst.value->setName("dst");

            metadata = generateCommon(builder, args, thunkSrcs, kernelDst, start, stop);

            dst.type = kernelDst;
            builder.CreateRetVoid();
        }

        builder.SetInsertPoint(allocation);

        static FunctionType *likelyForkType = NULL;
        if (!likelyForkType) {
            Type *params[] = { thunk->getType(), PointerType::get(MatType::MultiDimension, 0), NativeInt };
            likelyForkType = FunctionType::get(Type::getVoidTy(C), params, false);
        }

        Function *likelyFork = builder.module()->getFunction("likely_fork");
        if (!likelyFork) {
            likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "likely_fork", builder.module());
            likelyFork->setCallingConv(CallingConv::C);
            likelyFork->setDoesNotCapture(1);
            likelyFork->setDoesNotAlias(1);
            likelyFork->setDoesNotCapture(2);
            likelyFork->setDoesNotAlias(2);
        }

        Value *thunkMatrixArray = builder.CreateAlloca(MatType::MultiDimension, constant(srcs.size()+1));
        for (size_t i=0; i<srcs.size()+1; i++) {
            const likely_expression &src = i < srcs.size() ? srcs[i] : dst;
            Value *val;
            if (likely_multi_dimension(src)) {
               val = src;
            } else {
                val = builder.CreateAlloca(src.value->getType());
                builder.CreateStore(src, val);
            }
            builder.CreateStore(builder.CreatePointerCast(val, MatType::MultiDimension), builder.CreateGEP(thunkMatrixArray, constant(i)));
        }

        builder.CreateCall3(likelyFork, builder.module()->getFunction(thunk->getName()), thunkMatrixArray, kernelSize);
        return metadata;
    }

    Metadata generateHeterogeneous(Builder &, likely_const_ast, const vector<likely_expression> &, likely_expression &, Value *) const
    {
        assert(!"Not implemented");
        return Metadata();
    }

    static bool getPairs(likely_const_ast ast, vector<pair<likely_const_ast,likely_const_ast>> &pairs)
    {
        pairs.clear();
        if (!ast->is_list)       return false;
        if (ast->num_atoms == 0) return true;

        if (ast->atoms[0]->is_list) {
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

    static Value *getDimensions(Builder &builder, const vector<pair<likely_const_ast,likely_const_ast>> &pairs, const char *axis, const vector<likely_expression> &srcs, likely_type *type)
    {
        Value *result = NULL;
        for (const auto &pair : pairs) // Look for a dimensionality expression
            if (!strcmp(axis, pair.first->atom)) {
                result = builder.cast(unique_ptr<likely_expression>(builder.expression(pair.second)).get(), likely_matrix_native);
                break;
            }

        // Use default dimensionality
        if (result == NULL) {
            if (srcs.empty()) {
                result = constant(1);
            } else {
                if      (!strcmp(axis, "channels")) result = builder.channels(&srcs[0]);
                else if (!strcmp(axis, "columns"))  result = builder.columns (&srcs[0]);
                else if (!strcmp(axis, "rows"))     result = builder.rows    (&srcs[0]);
                else                                result = builder.frames  (&srcs[0]);
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
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        return new Kernel(builder, ast);
    }
};
LIKELY_REGISTER_EXPRESSION(kernel, "=>")

struct Reduction : public Kernel
{
    Reduction(Builder &builder, likely_const_ast ast)
        : Kernel(builder, ast) {}

    virtual Metadata generateCommon(Builder &builder, likely_const_ast args, const vector<likely_expression> &srcs, likely_expression &dst, Value *start, Value *stop) const
    {
        vector<Loop> loops;
        if (likely_multi_frame  (srcs[0]) && !likely_multi_frame  (dst)) loops.push_back(Loop(builder, "t", zero(), builder.frames  (&srcs[0])));
        if (likely_multi_row    (srcs[0]) && !likely_multi_row    (dst)) loops.push_back(Loop(builder, "y", zero(), builder.rows    (&srcs[0])));
        if (likely_multi_column (srcs[0]) && !likely_multi_column (dst)) loops.push_back(Loop(builder, "x", zero(), builder.columns (&srcs[0])));
        if (likely_multi_channel(srcs[0]) && !likely_multi_channel(dst)) loops.push_back(Loop(builder, "c", zero(), builder.channels(&srcs[0])));

        for (size_t i=0; i<loops.size(); i++)
            builder.define(loops[i].name.c_str(), &loops[i]);

        Metadata metadata = Kernel::generateCommon(builder, args, srcs, dst, start, stop);

        for (vector<Loop>::reverse_iterator it = loops.rbegin(); it != loops.rend(); it++) {
            builder.undefine(it->name.c_str());
            it->close(builder);
        }

        return metadata;
    }
};

class reductionExpression : public Operator
{
    size_t minParameters() const { return 2; }
    size_t maxParameters() const { return 3; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        return new Reduction(builder, ast);
    }
};
LIKELY_REGISTER_EXPRESSION(reduction, "+>")

class defineExpression : public Operator
{
    size_t maxParameters() const { return 2; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast lhs = ast->atoms[1];
        likely_const_ast rhs = ast->atoms[2];
        const char *name = lhs->is_list ? lhs->atoms[0]->atom : lhs->atom;

        if (likely_definition(builder.env->type)) {
            likely_expression *value;
            if (lhs->is_list) {
                // Export symbol
                vector<likely_type> types;
                for (size_t i=1; i<lhs->num_atoms; i++) {
                    if (lhs->atoms[i]->is_list)
                        return error(lhs->atoms[i], "expected an atom name parameter type");
                    types.push_back(likely_type_from_string(lhs->atoms[i]->atom));
                }

                if (builder.lookupResources()) {
                    // Offline
                    TRY_EXPR(builder, rhs, expr);
                    Lambda *lambda = static_cast<Lambda*>(expr.get());
                    value = lambda->generate(builder, types, name, false);
                } else {
                    // JIT
                    JITFunction *function = new JITFunction(name, rhs, builder.env, types, false);
                    sys::DynamicLibrary::AddSymbol(name, function->function);
                    value = function;
                }
            } else {
                // Global variable
                value = new Definition(builder, rhs);
            }

            builder.env->name = new char[strlen(name)+1];
            strcpy((char*) builder.env->name, name);
            builder.env->value = value;
            return NULL;
        } else {
            // Local variable
            likely_expression *expr = builder.expression(rhs);
            if (expr) {
                shared_ptr<likely_expression> &variable = builder.locals[name];
                if (variable.get()) static_cast<Variable*>(variable.get())->set(builder, expr);
                else                variable.reset(new Variable(builder, expr, name));
            }
            return expr;
        }
    }
};
LIKELY_REGISTER_EXPRESSION(define, "=")

JITFunction::JITFunction(const string &name, likely_const_ast ast, likely_env parent, const vector<likely_type> &parameters, bool arrayCC)
    : resources(true), parameters(parameters)
{
    likely_assert(ast->is_list && (ast->num_atoms > 0) && !ast->atoms[0]->is_list &&
                  (!strcmp(ast->atoms[0]->atom, "->") || !strcmp(ast->atoms[0]->atom, "=>") || !strcmp(ast->atoms[0]->atom, "+>")),
                  "expected a lambda expression");

    Builder builder(NULL);
    ScopedEnvironment se(builder, parent, &resources);
    unique_ptr<likely_expression> result(builder.expression(ast));
    unique_ptr<Symbol> expr(static_cast<Lambda*>(result.get())->generate(builder, parameters, name, arrayCC));
    value = expr->value;
    type = expr->type;

    if (!value)
        return;

    string error;
    EngineBuilder engineBuilder(resources.module);
    engineBuilder.setEngineKind(EngineKind::JIT)
            .setErrorStr(&error)
            .setUseMCJIT(true);
    EE = engineBuilder.create(likely_resources::getTargetMachine(true));
    EE->setObjectCache(&TheJITFunctionCache);
    likely_assert(EE != NULL, "failed to create execution engine with error: %s", error.c_str());

    if (!TheJITFunctionCache.alert(resources.module)) {
        static PassManager *PM = NULL;
        if (!PM) {
            static TargetMachine *TM = likely_resources::getTargetMachine(false);
            PM = new PassManager();
            PM->add(createVerifierPass());
            PM->add(new TargetLibraryInfo(Triple(resources.module->getTargetTriple())));
            PM->add(new DataLayoutPass(*TM->getDataLayout()));
            TM->addAnalysisPasses(*PM);
            PassManagerBuilder builder;
            builder.OptLevel = 3;
            builder.SizeLevel = 0;
            builder.LoopVectorize = true;
            builder.Inliner = createAlwaysInlinerPass();
            builder.populateModulePassManager(*PM);
            PM->add(createVerifierPass());
        }

//        DebugFlag = true;
//        module->dump();
        PM->run(*resources.module);
//        module->dump();
    }
    hash = TheJITFunctionCache.currentHash;

    EE->finalizeObject();
    function = EE->getPointerToFunction(dyn_cast<Function>(expr->value));
}

#ifdef LIKELY_IO
#include "likely/io.h"

class printExpression : public VAOperator
{
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        static FunctionType *functionType = FunctionType::get(MatType::MultiDimension, MatType::MultiDimension, true);
        Function *likelyPrint = builder.module()->getFunction("likely_print");
        if (!likelyPrint) {
            likelyPrint = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_print", builder.module());
            likelyPrint->setCallingConv(CallingConv::C);
            likelyPrint->setDoesNotAlias(0);
            likelyPrint->setDoesNotAlias(1);
            likelyPrint->setDoesNotCapture(1);
        }

        vector<Value*> rawArgs;
        for (size_t i=1; i<ast->num_atoms; i++) {
            TRY_EXPR(builder, ast->atoms[i], arg);
            rawArgs.push_back(*arg);
        }
        rawArgs.push_back(nullMat());

        vector<Value*> matArgs;
        for (Value *rawArg : rawArgs)
            if (rawArg->getType() == MatType::MultiDimension) {
                matArgs.push_back(rawArg);
            } else {
                matArgs.push_back(stringExpression::createCall(builder, rawArg));
            }

        Value *result = builder.CreateCall(likelyPrint, matArgs);

        for (size_t i=0; i<rawArgs.size(); i++)
            if (rawArgs[i] != matArgs[i])
                releaseExpression::createCall(builder, matArgs[i]);

        return new likely_expression(result, likely_matrix_i8);
    }

    void *symbol() const { return (void*) likely_print; }
};
LIKELY_REGISTER(print)

class readExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type* params[] = { Type::getInt8PtrTy(C), Type::getInt1Ty(C) };
            functionType = FunctionType::get(MatType::MultiDimension, params, false);
        }

        Function *likelyRead = builder.module()->getFunction("likely_read");
        if (!likelyRead) {
            likelyRead = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_read", builder.module());
            likelyRead->setCallingConv(CallingConv::C);
            likelyRead->setDoesNotAlias(0);
            likelyRead->setDoesNotAlias(1);
            likelyRead->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall2(likelyRead, *arg, ConstantInt::getTrue(C)), MatType::MultiDimension);
    }
    void *symbol() const { return (void*) likely_read; }
};
LIKELY_REGISTER(read)

class writeExpression : public SimpleBinaryOperator
{
    likely_expression *evaluateSimpleBinary(Builder &builder, const unique_ptr<likely_expression> &arg1, const unique_ptr<likely_expression> &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type* params[] = { MatType::MultiDimension, Type::getInt8PtrTy(C) };
            functionType = FunctionType::get(MatType::MultiDimension, params, false);
        }
        Function *likelyWrite = builder.module()->getFunction("likely_write");
        if (!likelyWrite) {
            likelyWrite = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_write", builder.module());
            likelyWrite->setCallingConv(CallingConv::C);
            likelyWrite->setDoesNotAlias(0);
            likelyWrite->setDoesNotAlias(1);
            likelyWrite->setDoesNotCapture(1);
            likelyWrite->setDoesNotAlias(2);
            likelyWrite->setDoesNotCapture(2);
        }
        return new likely_expression(builder.CreateCall2(likelyWrite, *arg1, *arg2), MatType::MultiDimension);
    }
    void *symbol() const { return (void*) likely_write; }
};
LIKELY_REGISTER(write)

class decodeExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        static FunctionType *functionType = FunctionType::get(MatType::MultiDimension, MatType::MultiDimension, false);
        Function *likelyDecode = builder.module()->getFunction("likely_decode");
        if (!likelyDecode) {
            likelyDecode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_decode", builder.module());
            likelyDecode->setCallingConv(CallingConv::C);
            likelyDecode->setDoesNotAlias(0);
            likelyDecode->setDoesNotAlias(1);
            likelyDecode->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyDecode, *arg), MatType::MultiDimension);
    }
    void *symbol() const { return (void*) likely_decode; }
};
LIKELY_REGISTER(decode)

class encodeExpression : public SimpleBinaryOperator
{
    likely_expression *evaluateSimpleBinary(Builder &builder, const unique_ptr<likely_expression> &arg1, const unique_ptr<likely_expression> &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type* params[] = { MatType::MultiDimension, Type::getInt8PtrTy(C) };
            functionType = FunctionType::get(MatType::MultiDimension, params, false);
        }
        Function *likelyEncode = builder.module()->getFunction("likely_encode");
        if (!likelyEncode) {
            likelyEncode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_encode", builder.module());
            likelyEncode->setCallingConv(CallingConv::C);
            likelyEncode->setDoesNotAlias(0);
            likelyEncode->setDoesNotAlias(1);
            likelyEncode->setDoesNotCapture(1);
            likelyEncode->setDoesNotAlias(2);
            likelyEncode->setDoesNotCapture(2);
        }
        return new likely_expression(builder.CreateCall2(likelyEncode, *arg1, *arg2), MatType::MultiDimension);
    }
    void *symbol() const { return (void*) likely_encode; }
};
LIKELY_REGISTER(encode)

class renderExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type* params[] = { MatType::MultiDimension, Type::getDoublePtrTy(C), Type::getDoublePtrTy(C) };
            functionType = FunctionType::get(MatType::MultiDimension, params, false);
        }
        Function *likelyRender = builder.module()->getFunction("likely_render");
        if (!likelyRender) {
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
        return new likely_expression(builder.CreateCall3(likelyRender, *arg, ConstantPointerNull::get(Type::getDoublePtrTy(C)), ConstantPointerNull::get(Type::getDoublePtrTy(C))), MatType::MultiDimension);
    }
    void *symbol() const { return (void*) likely_render; }
};
LIKELY_REGISTER(render)

class showExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const unique_ptr<likely_expression> &arg) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type* params[] = { MatType::MultiDimension, Type::getInt8PtrTy(C) };
            FunctionType::get(Type::getVoidTy(C), params, false);
        }
        Function *likelyShow = builder.module()->getFunction("likely_show");
        if (!likelyShow) {
            likelyShow = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_show", builder.module());
            likelyShow->setCallingConv(CallingConv::C);
            likelyShow->setDoesNotAlias(1);
            likelyShow->setDoesNotCapture(1);
            likelyShow->setDoesNotAlias(2);
            likelyShow->setDoesNotCapture(2);
        }
        return new likely_expression(builder.CreateCall2(likelyShow, *arg, ConstantPointerNull::get(Type::getInt8PtrTy(C))), MatType::MultiDimension);
    }
    void *symbol() const { return (void*) likely_decode; }
};
LIKELY_REGISTER(show)
#endif // LIKELY_IO

} // namespace (anonymous)

likely_env likely_new_env(likely_const_env parent)
{
    likely_env env = (likely_env) malloc(sizeof(likely_environment));
    env->parent = likely_retain_env(parent);
    env->name = NULL;
    env->value = NULL;
    env->resources = NULL;
    env->result = NULL;
    env->ref_count = 1;
    env->hash = 0;
    env->type = likely_environment_void;
    return env;
}

likely_env likely_new_env_jit()
{
    return likely_new_env(RootEnvironment);
}

likely_env likely_new_env_offline(const char *file_name, bool native)
{
    likely_env env = likely_new_env(RootEnvironment);
    env->resources = new OfflineResources(file_name, native);
    likely_set_offline(&env->type, true);
    return env;
}

likely_env likely_retain_env(likely_const_env env)
{
    if (env) const_cast<likely_env>(env)->ref_count++;
    return (likely_env) env;
}

void likely_release_env(likely_const_env env)
{
    if (!env || --const_cast<likely_env>(env)->ref_count) return;
    likely_release_env(env->parent);
    delete[] env->name;
    if (!likely_definition(env->type))
        likely_release(env->result);
    delete env->resources;
    free((void*) env);
}

bool likely_offline(likely_environment_type type) { return likely_get_bool(type, likely_environment_offline); }
void likely_set_offline(likely_environment_type *type, bool offline) { likely_set_bool(type, offline, likely_environment_offline); }
bool likely_erratum(likely_environment_type type) { return likely_get_bool(type, likely_environment_erratum); }
void likely_set_erratum(likely_environment_type *type, bool erratum) { likely_set_bool(type, erratum, likely_environment_erratum); }
bool likely_definition(likely_environment_type type) { return likely_get_bool(type, likely_environment_definition); }
void likely_set_definition(likely_environment_type *type, bool definition) { likely_set_bool(type, definition, likely_environment_definition); }

likely_mat likely_dynamic(likely_vtable vtable, likely_const_mat *mv)
{
    void *function = NULL;
    for (size_t i=0; i<vtable->functions.size(); i++) {
        const unique_ptr<JITFunction> &resources = vtable->functions[i];
        for (size_t j=0; j<vtable->n; j++)
            if (mv[j]->type != resources->parameters[j])
                goto Next;
        function = resources->function;
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
        vtable->functions.push_back(unique_ptr<JITFunction>(new JITFunction("likely_vtable_entry", vtable->ast, vtable->env, types, true)));
        function = vtable->functions.back()->function;
        if (function == NULL)
            return NULL;
    }

    return reinterpret_cast<likely_function_n>(function)(mv);
}

static map<likely_function, JITFunction*> JITFunctionLUT;

likely_function likely_compile(likely_const_ast ast, likely_env env, likely_type type, ...)
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
    JITFunction *jit = new JITFunction("likely_jit_function", ast, env, types, false);
    likely_function f = reinterpret_cast<likely_function>(jit->function);
    if (f) JITFunctionLUT[f] = jit;
    else   delete jit;
    return f;
}

likely_function likely_retain_function(likely_function function)
{
    if (function) JITFunctionLUT[function]->ref_count++;
    return function;
}

void likely_release_function(likely_function function)
{
    if (!function) return;
    JITFunction*& jit = JITFunctionLUT[function];
    if (--jit->ref_count) return;
    JITFunctionLUT.erase(function);
    delete jit;
}

likely_env likely_eval(likely_const_ast ast, likely_const_env parent, likely_const_env previous)
{
    if (!ast || !parent) return NULL;
    likely_env env = likely_new_env(parent);
    likely_set_offline(&env->type, likely_offline(parent->type));
    likely_set_definition(&env->type, ast->is_list && (ast->num_atoms > 0) && !strcmp(ast->atoms[0]->atom, "="));
    if (likely_offline(env->type)) {
        Builder builder(env);
        unique_ptr<likely_expression> e(builder.expression(ast));
        likely_set_erratum(&env->type, e.get() == NULL);
    } else {
        if (likely_definition(env->type)) {
            // Shortcut for global variable definitions
            delete Builder(env).expression(ast);
        } else {
            likely_const_ast lambda = likely_ast_from_string("() -> (scalar <ast>)", false);
            likely_release_ast(lambda->atoms[0]->atoms[2]->atoms[1]); // <ast>
            lambda->atoms[0]->atoms[2]->atoms[1] = likely_retain_ast(ast);
            JITFunction jit("likely_jit_function", lambda->atoms[0], env, vector<likely_type>(), false);
            if (likely_function function = jit.function) {
                env->hash = jit.hash;
                likely_const_env it = previous;
                while (it) {
                    if (it->hash == env->hash) {
                        env->result = likely_retain(it->result);
                        break;
                    }
                    it = it->parent;
                }
                if (!env->result)
                    env->result = reinterpret_cast<likely_function_0>(function)();
            } else {
                likely_set_erratum(&env->type, true);
            }
            likely_release_ast(lambda);
        }
    }
    return env;
}

likely_env likely_repl(const char *source, bool GFM, likely_const_env parent, likely_const_env previous)
{
    likely_const_ast ast = likely_ast_from_string(source, GFM);
    if (!ast)
        return NULL;

    likely_env env = likely_retain_env(parent);
    for (size_t i=0; i<ast->num_atoms; i++) {
        likely_const_ast atom = ast->atoms[i];
        env = likely_eval(atom, parent, previous);
        likely_release_env(parent);
        parent = env;
        if (!likely_definition(env->type) && env->result && (likely_elements(env->result) > 0))
            likely_show(env->result, atom);
        if (likely_erratum(env->type))
            break;
    }

    likely_release_ast(ast);
    return env;
}

namespace {

class importExpression : public Operator
{
    size_t maxParameters() const { return 1; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast file = ast->atoms[1];
        if (file->is_list)
            return error(file, "expected a file name");

        const string fileName = string(file->atom) + ".l";
        ifstream stream(fileName);
        const string source((istreambuf_iterator<char>(stream)),
                             istreambuf_iterator<char>());
        if (source.empty())
            return error(file, "unable to open file");

        likely_env parent = builder.env;
        builder.env = likely_repl(source.c_str(), true, parent, NULL);
        likely_release_env(parent);
        if (likely_erratum(builder.env->type)) return NULL;
        else                                   return new likely_expression();
    }
};
LIKELY_REGISTER(import)

} // namespace (anonymous)
