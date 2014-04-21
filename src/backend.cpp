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

#include <llvm/PassManager.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Analysis/Passes.h>
#include <llvm/Analysis/TargetTransformInfo.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Target/TargetLibraryInfo.h>
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
#include <sstream>

#include "likely/backend.h"

#define LLVM_VALUE_IS_INT(VALUE) (llvm::isa<Constant>(VALUE))
#define LLVM_VALUE_TO_INT(VALUE) (llvm::cast<Constant>(VALUE)->getUniqueInteger().getZExtValue())

using namespace llvm;
using namespace std;

namespace {

static IntegerType *NativeInt = NULL;
static LLVMContext &C = getGlobalContext();

struct T
{
    Type *llvm;
    likely_type likely;
    static PointerType *Void;

    T() : llvm(NULL), likely(likely_type_void) {}

    T(Type *llvm, likely_type likely)
        : llvm(llvm), likely(likely) {}

    static T get(likely_type likely)
    {
        auto result = likelyLUT.find(likely);
        if (result != likelyLUT.end())
            return result->second;

        likely_mat str = likely_type_to_string(likely);
        const string name = string("mat_") + (const char*) str->data;
        PointerType *mat = PointerType::getUnqual(StructType::create(name,
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

        T t(mat, likely);
        likelyLUT[t.likely] = t;
        llvmLUT[t.llvm] = t;
        return t;
    }

    static T get(Type *llvm)
    {
        auto result = llvmLUT.find(llvm);
        likely_assert(result != llvmLUT.end(), "invalid pointer for type lookup");
        return result->second;
    }

    vector<Type*> static toLLVM(const vector<T> &tl)
    {
        vector<Type*> result;
        for (const T &t : tl)
            result.push_back(t.llvm);
        return result;
    }

private:
    static map<likely_type, T> likelyLUT;
    static map<Type*, T> llvmLUT;
};
PointerType *T::Void = NULL;
map<likely_type, T> T::likelyLUT;
map<Type*, T> T::llvmLUT;

struct Builder;
struct likely_expression
{
    likely_expression() : value_(NULL), type_(likely_type_void) {}
    likely_expression(Value *value, likely_type type)
        : value_(value), type_(type) {}
    virtual ~likely_expression() {}

    virtual bool isNull() const
    {
        return !value_;
    }

    virtual Value *value() const
    {
        likely_assert(value_ != NULL, "Expression has no value!");
        return value_;
    }

    virtual likely_type type() const
    {
        likely_assert(value_ != NULL, "Expression has no type!");
        return type_;
    }

    void setType(likely_type type)
    {
        type_ = type;
    }

    virtual likely_expression *evaluate(Builder &, likely_const_ast) const
    {
        return new likely_expression(value(), type());
    }

    virtual int rightHandAtoms() const { return 1; }

    operator Value*() const { return value(); }
    operator likely_type() const { return type(); }

    Value *take()
    {
        Value *result = value();
        delete this; // With great power comes great responsibility
        return result;
    }

    static likely_expression *error(likely_const_ast ast, const char *message)
    {
        likely_throw(ast, message);
        return NULL;
    }

private:
    Value *value_;
    likely_type type_;
};

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

struct UniqueExpression : public likely_expression, public unique_ptr<likely_expression>
{
    UniqueExpression(likely_expression *e = NULL)
        : unique_ptr<likely_expression>(e) {}
    bool isNull() const { return get()->isNull(); }
    Value* value() const { return get()->value(); }
    likely_type type() const { return get()->type(); }
    likely_expression *evaluate(Builder &builder, likely_const_ast ast) const
        { return get()->evaluate(builder, ast); }
};

#define TRY_EXPR(BUILDER, AST, EXPR)                    \
const UniqueExpression EXPR((BUILDER).expression(AST)); \
if (!EXPR.get()) return NULL;                           \

static string getUniqueName(const string &prefix)
{
    static map<string, int> uidLUT;
    stringstream stream;
    stream << "likely_" << prefix << "_" << uidLUT[prefix]++;
    return stream.str();
}

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

            NativeInt = Type::getIntNTy(C, likely_depth(likely_type_native));
            T::Void = cast<PointerType>(T::get(likely_type_void).llvm);
        }

        module = new Module(getUniqueName("module"), C);
        likely_assert(module != NULL, "failed to create module");
        if (native) module->setTargetTriple(sys::getProcessTriple());
    }

    void optimize()
    {
        if (module->getTargetTriple().empty())
            return;

        static TargetMachine *TM = getTargetMachine(false);
        static PassManager *PM = NULL;
        if (!PM) {
            PM = new PassManager();
            PM->add(createVerifierPass());
            PM->add(new TargetLibraryInfo(Triple(module->getTargetTriple())));
            PM->add(new DataLayoutPass(*TM->getDataLayout()));
            TM->addAnalysisPasses(*PM);
            PassManagerBuilder builder;
            builder.OptLevel = 3;
            builder.SizeLevel = 0;
            builder.LoopVectorize = true;
            builder.populateModulePassManager(*PM);
            PM->add(createVerifierPass());
        }

//        DebugFlag = true;
//        module->dump();
        PM->run(*module);
//        module->dump();
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

class JITFunction : public likely_resources
{
    ExecutionEngine *EE = NULL;

public:
    void *function = NULL;
    bool error = false;
    const vector<likely_type> type;

    JITFunction(likely_const_ast ast, likely_env env, const vector<likely_type> &type);

    ~JITFunction()
    {
        module = NULL;
        delete EE; // owns module
    }
};

class OfflineResources : public likely_resources
{
    const string fileName;

public:
    OfflineResources(const string &fileName, bool native)
        : likely_resources(native), fileName(fileName)
    {}

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

} // namespace (anonymous)

struct likely_environment
{
    likely_env parent;
    const char *name;
    likely_expression *value;
    likely_const_mat result;
    likely_resources *resources;
    size_t ref_count;

    virtual ~likely_environment() {}
    virtual likely_env evaluate(likely_const_ast ast);

#ifdef __cplusplus
    likely_environment(likely_env parent)
        : parent(likely_retain_env(parent)), name(NULL), value(NULL), result(NULL), resources(NULL), ref_count(1) {}

private:
    likely_environment(const likely_environment &);
    likely_environment &operator=(const likely_environment &);
#endif // __cplusplus
};

namespace {

static likely_env RootEnvironment = new likely_environment(NULL);

struct Builder : public IRBuilder<>
{
    likely_env env;
    map<string, shared_ptr<likely_expression>> locals;

    Builder(likely_env env)
        : IRBuilder<>(C), env(likely_retain_env(env)) {}

    ~Builder() { likely_release_env(env); }

    static likely_expression constant(uint64_t value, likely_type type = likely_type_native)
    {
        const int depth = likely_depth(type);
        return likely_expression(Constant::getIntegerValue(Type::getIntNTy(C, depth), APInt(depth, value)), type);
    }

    static likely_expression constant(int value, likely_type type)
    {
        return constant(uint64_t(value), type);
    }

    static likely_expression constant(double value, likely_type type)
    {
        const int depth = likely_depth(type);
        if (likely_floating(type)) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return likely_expression(ConstantFP::get(Type::getDoubleTy(C), value), type);
            else if (depth == 32) return likely_expression(ConstantFP::get(Type::getFloatTy(C), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return likely_expression(NULL, likely_type_void); }
        } else {
            return constant(uint64_t(value), type);
        }
    }

    static likely_expression zero(likely_type type = likely_type_native) { return constant(0, type); }
    static likely_expression one (likely_type type = likely_type_native) { return constant(1, type); }
    static likely_expression intMax(likely_type type) { const int bits = likely_depth(type); return constant((1 << (bits - (likely_signed(type) ? 1 : 0)))-1, bits); }
    static likely_expression intMin(likely_type type) { const int bits = likely_depth(type); return constant(likely_signed(type) ? (1 << (bits - 1)) : 0, bits); }
    static likely_expression type(likely_type type) { return constant(uint64_t(type), likely_type_type); }
    static likely_expression nullMat() { return likely_expression(ConstantPointerNull::get(T::Void), likely_type_void); }
    static likely_expression nullData() { return likely_expression(ConstantPointerNull::get(Type::getInt8PtrTy(C)), likely_type_native); }

    likely_expression channels(const likely_expression *matrix) { return likely_multi_channel(*matrix) ? likely_expression(CreateLoad(CreateStructGEP(*matrix, 2), "channels"), likely_type_native) : one(); }
    likely_expression columns (const likely_expression *matrix) { return likely_multi_column (*matrix) ? likely_expression(CreateLoad(CreateStructGEP(*matrix, 3), "columns" ), likely_type_native) : one(); }
    likely_expression rows    (const likely_expression *matrix) { return likely_multi_row    (*matrix) ? likely_expression(CreateLoad(CreateStructGEP(*matrix, 4), "rows"    ), likely_type_native) : one(); }
    likely_expression frames  (const likely_expression *matrix) { return likely_multi_frame  (*matrix) ? likely_expression(CreateLoad(CreateStructGEP(*matrix, 5), "frames"  ), likely_type_native) : one(); }
    likely_expression data    (const likely_expression *matrix) { return likely_expression(CreatePointerCast(CreateStructGEP(*matrix, 7), ty(*matrix, true)), likely_data(*matrix)); }

    void steps(const likely_expression *matrix, Value **columnStep, Value **rowStep, Value **frameStep)
    {
        *columnStep = channels(matrix);
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
                type = validFloatType(type);
        }
        Type *dstType = ty(type);
        return likely_expression(CreateCast(CastInst::getCastOpcode(*x, likely_signed(*x), dstType, likely_signed(type)), *x, dstType), type);
    }

    static Type *ty(likely_type type, bool pointer = false)
    {
        const int bits = likely_depth(type);
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

    static likely_type validFloatType(likely_type type)
    {
        likely_set_floating(&type, true);
        likely_set_signed(&type, true);
        likely_set_depth(&type, likely_depth(type) > 32 ? 64 : 32);
        return type;
    }

    vector<likely_expression> getArgs(Function *function, const vector<T> &types)
    {
        vector<likely_expression> result;
        Function::arg_iterator args = function->arg_begin();
        likely_arity n = 0;
        while (args != function->arg_end()) {
            Value *src = args++;
            stringstream name; name << "arg_" << int(n);
            src->setName(name.str());
            result.push_back(likely_expression(src, n < types.size() ? types[n].likely : likely_type(likely_type_void)));
            n++;
        }
        return result;
    }

    static likely_expression *lookup(likely_const_env env, const char *name)
    {
        if      (!env)                                  return NULL;
        else if (env->name && !strcmp(env->name, name)) return env->value;
        else                                            return lookup(env->parent, name);
    }

    likely_expression *lookup(const char *name) const { return lookup(env, name); }

    static likely_resources *lookupResources(likely_const_env env)
    {
        if      (!env)           return NULL;
        else if (env->resources) return env->resources;
        else                     return lookupResources(env->parent);
    }

    likely_resources *lookupResources() const { return lookupResources(env); }

    static void define(likely_env &env, const char *name, likely_expression *e)
    {
        env = new likely_environment(env);
        likely_release_env(env->parent);
        env->name = new char[strlen(name)+1];
        strcpy((char*) env->name, name);
        env->value = e;
        env->resources = lookupResources(env->parent);
    }

    static void undefine(likely_env &env, const char *name)
    {
        likely_assert(!strcmp(env->name, name), "undefine variable mismatch");
        likely_env old = env;
        env = likely_retain_env(env->parent);
        likely_release_env(old);
    }

    void   define(const char *name, likely_expression *e) {   define(env, name, e); }
    void undefine(const char *name)                       { undefine(env, name); }

    Module *module() { return lookupResources()->module; }

    likely_expression *expression(likely_const_ast ast);
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

} // namespace (anonymous)

struct VTable : public ScopedExpression
{
    size_t n;
    vector<unique_ptr<JITFunction>> functions;

    VTable(Builder &builder, likely_const_ast ast, size_t n)
        : ScopedExpression(builder, ast), n(n) {}

    likely_expression *evaluateOperator(Builder &, likely_const_ast) const { return NULL; }
};

extern "C" LIKELY_EXPORT likely_mat likely_dynamic(struct VTable *vtable, likely_const_mat m, ...);

namespace {

class LibraryFunction
{
    virtual void *symbol() const = 0; // Idiom to ensure that the library symbol isn't stripped when optimizing executable size
};

static int getPrecedence(const char *op)
{
    if (!strcmp(op, "=" )) return 1;
    if (!strcmp(op, "->")) return 2;
    if (!strcmp(op, "=>")) return 2;
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

template <class E>
struct RegisterExpression
{
    RegisterExpression(const char *symbol)
    {
        likely_expression *e = new E();
        Builder::define(RootEnvironment, symbol, e);
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
    virtual likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const = 0;
};

struct MatrixType : public SimpleUnaryOperator
{
    likely_type t;
    MatrixType(likely_type t)
        : t(t) {}

    Value *value() const { return Builder::type(t); }
    likely_type type() const { return likely_type_type; }

    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &x) const
    {
        return new likely_expression(builder.cast(&x, t));
    }
};

likely_expression *Builder::expression(likely_const_ast ast)
{
    if (ast->is_list) {
        if (ast->num_atoms == 0)
            return likely_expression::error(ast, "Empty expression");
        likely_const_ast op = ast->atoms[0];
        if (!op->is_list)
            if (likely_expression *e = lookup(op->atom))
                return e->evaluate(*this, ast);
        TRY_EXPR(*this, op, e);
        return e.evaluate(*this, ast);
    }
    const string op = ast->atom;

    { // Is it a local variable?
        auto var = locals.find(op);
        if (var != locals.end())
            return var->second->evaluate(*this, ast);
    }

    if (likely_expression *e = lookup(op.c_str()))
        return e->evaluate(*this, ast);

    if ((op.front() == '"') && (op.back() == '"'))
        return new likely_expression(CreateGlobalStringPtr(op.substr(1, op.length()-2)), likely_type_u8);

    { // Is it a number?
        char *p;
        const double value = strtod(op.c_str(), &p);
        if (*p == 0)
            return new likely_expression(constant(value, likely_type_from_value(value)));
    }

    { // Is it a type?
        bool ok;
        likely_type type = likely_type_field_from_string(op.c_str(), &ok);
        if (ok) return new MatrixType(type);
    }

    return likely_expression::error(ast, "unrecognized literal");
}

#define LIKELY_REGISTER_FIELD(FIELD)                                                            \
class FIELD##Expression : public SimpleUnaryOperator                                            \
{                                                                                               \
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const \
    {                                                                                           \
        return new likely_expression(builder.FIELD(&arg));                                      \
    }                                                                                           \
};                                                                                              \
LIKELY_REGISTER(FIELD)                                                                          \

LIKELY_REGISTER_FIELD(channels)
LIKELY_REGISTER_FIELD(columns)
LIKELY_REGISTER_FIELD(rows)
LIKELY_REGISTER_FIELD(frames)

class notExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new likely_expression(builder.CreateXor(Builder::intMax(arg), arg.value()), arg);
    }
};
LIKELY_REGISTER_EXPRESSION(not, "~")

class typeExpression : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &, const UniqueExpression &arg) const
    {
        return new MatrixType(arg);
    }
};
LIKELY_REGISTER(type)

class UnaryMathOperator : public SimpleUnaryOperator
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &x) const
    {
        likely_expression xc(builder.cast(&x, Builder::validFloatType(x)));
        vector<Type*> args;
        args.push_back(xc.value()->getType());
        return new likely_expression(builder.CreateCall(Intrinsic::getDeclaration(builder.module(), id(), args), xc), xc);
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
    virtual likely_expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &arg1, const UniqueExpression &arg2) const = 0;
};

class ArithmeticOperator : public SimpleBinaryOperator
{
    likely_expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &lhs, const UniqueExpression &rhs) const
    {
        likely_type type = likely_type_from_types(lhs, rhs);
        return evaluateArithmetic(builder, builder.cast(&lhs, type), builder.cast(&rhs, type));
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
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), likely_signed(lhs) ? Intrinsic::sadd_with_overflow : Intrinsic::uadd_with_overflow, lhs.value()->getType()), lhs, rhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, Builder::zero(lhs)), Builder::intMax(lhs), Builder::intMin(lhs)) : Builder::intMax(lhs);
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
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), likely_signed(lhs) ? Intrinsic::ssub_with_overflow : Intrinsic::usub_with_overflow, lhs.value()->getType()), lhs, rhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateICmpSGE(lhs.value(), Builder::zero(lhs)), Builder::intMax(lhs), Builder::intMin(lhs)) : Builder::intMin(lhs);
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
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), likely_signed(lhs) ? Intrinsic::smul_with_overflow : Intrinsic::umul_with_overflow, lhs.value()->getType()), lhs, rhs);
                Value *zero = Builder::zero(lhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateXor(builder.CreateICmpSGE(lhs.value(), zero), builder.CreateICmpSGE(rhs.value(), zero)), Builder::intMin(lhs), Builder::intMax(lhs)) : Builder::intMax(lhs);
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
                    Value *safe_i = builder.CreateAdd(n, builder.CreateZExt(builder.CreateICmpNE(builder.CreateOr(builder.CreateAdd(d.value(), Builder::one(n)), builder.CreateAdd(n.value(), Builder::intMin(n))), Builder::zero(n)), n.value()->getType()));
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
        return builder.Create##OP(lhs, rhs.value());                                                                    \
    }                                                                                                                   \
};                                                                                                                      \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                     \

LIKELY_REGISTER_LOGIC(And , "&"   )
LIKELY_REGISTER_LOGIC(Or  , "|"   )
LIKELY_REGISTER_LOGIC(Xor , "^"   )
LIKELY_REGISTER_LOGIC(Shl , "<<"  )
LIKELY_REGISTER_LOGIC(LShr, "lshr")
LIKELY_REGISTER_LOGIC(AShr, "ashr")

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                            \
class OP##Expression : public ArithmeticOperator                                                                                       \
{                                                                                                                                      \
    likely_expression *evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const          \
    {                                                                                                                                  \
        return new likely_expression(likely_floating(lhs) ? builder.CreateFCmpO##OP(lhs, rhs)                                          \
                                                          : (likely_signed(lhs) ? builder.CreateICmpS##OP(lhs, rhs)                    \
                                                                                : builder.CreateICmpU##OP(lhs, rhs)), likely_type_u1); \
    }                                                                                                                                  \
};                                                                                                                                     \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                                    \

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
                                                          : builder.CreateICmp##OP(lhs, rhs), likely_type_u1);                \
    }                                                                                                                         \
};                                                                                                                            \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                           \

LIKELY_REGISTER_EQUALITY(EQ, "==")
LIKELY_REGISTER_EQUALITY(NE, "!=")

class BinaryMathOperator : public SimpleBinaryOperator
{
    likely_expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &x, const UniqueExpression &n) const
    {
        const likely_type type = nIsInteger() ? x.type() : likely_type_from_types(x, n);
        likely_expression xc(builder.cast(&x, Builder::validFloatType(type)));
        likely_expression nc(builder.cast(&n, nIsInteger() ? likely_type(likely_type_i32) : xc.type()));
        vector<Type*> args;
        args.push_back(xc.value()->getType());
        return new likely_expression(builder.CreateCall2(Intrinsic::getDeclaration(builder.module(), id(), args), xc, nc), xc);
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
        likely_env restored = builder.env;
        builder.env = new likely_environment(env);
        builder.env->resources = Builder::lookupResources(restored);
        UniqueExpression op(builder.expression(this->ast));
        likely_release_env(builder.env);
        builder.env = restored;
        return op->evaluate(builder, ast);
    }
};

class Variable : public Operator
{
    likely_type type_;
    AllocaInst *allocaInst;

public:
    Variable(Builder &builder, likely_expression *expr, const string &name)
    {
        type_ = *expr;
        allocaInst = builder.CreateAlloca(expr->value()->getType(), 0, name);
        set(builder, expr);
    }

    void set(Builder &builder, likely_expression *expr)
    {
        builder.CreateStore(builder.cast(expr, type_), allocaInst);
    }

private:
    size_t maxParameters() const { return 0; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast) const
    {
        return new likely_expression(builder.CreateLoad(allocaInst), type_);
    }
};

class DefinitionOperator : public Operator
{
    size_t maxParameters() const { return 2; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        if (ast->atoms[1]->is_list)
            return error(ast->atoms[1], "expected an atom");
        return evaluateDefinition(builder, ast->atoms[1], ast->atoms[2]);
    }
    virtual likely_expression *evaluateDefinition(Builder &builder, likely_const_ast name, likely_const_ast value) const = 0;
};

class defineExpression : public DefinitionOperator
{
    likely_expression *evaluateDefinition(Builder &builder, likely_const_ast name, likely_const_ast value) const
    {
        if (builder.env->resources) {
            // Local variable
            likely_expression *expr = builder.expression(value);
            if (expr) {
                shared_ptr<likely_expression> &variable = builder.locals[name->atom];
                if (variable.get()) static_cast<Variable*>(variable.get())->set(builder, expr);
                else                variable.reset(new Variable(builder, expr, name->atom));
            }
            return expr;
        } else {
            // Global variable
            builder.env->name = new char[strlen(name->atom)+1];
            strcpy((char*) builder.env->name, name->atom);
            builder.env->value = new Definition(builder, value);
            return NULL;
        }
    }
};
LIKELY_REGISTER_EXPRESSION(define, "=")

class definedExpression : public DefinitionOperator
{
    likely_expression *evaluateDefinition(Builder &builder, likely_const_ast name, likely_const_ast value) const
    {
        if (builder.lookup(name->atom)) return builder.expression(name);
        else                            return builder.expression(value);
    }
};
LIKELY_REGISTER_EXPRESSION(defined, "??")

class elementsExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(NativeInt, T::Void, false);
        Function *likelyElements = builder.module()->getFunction("likely_elements");
        if (!likelyElements) {
            likelyElements = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_elements", builder.module());
            likelyElements->setCallingConv(CallingConv::C);
            likelyElements->setDoesNotAlias(1);
            likelyElements->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyElements, arg), likely_type_native);
    }
    void *symbol() const { return (void*) likely_elements; }
};
LIKELY_REGISTER(elements)

class bytesExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(NativeInt, T::Void, false);
        Function *likelyBytes = builder.module()->getFunction("likely_bytes");
        if (!likelyBytes) {
            likelyBytes = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_bytes", builder.module());
            likelyBytes->setCallingConv(CallingConv::C);
            likelyBytes->setDoesNotAlias(1);
            likelyBytes->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyBytes, arg), likely_type_native);
    }
    void *symbol() const { return (void*) likely_bytes; }
};
LIKELY_REGISTER(bytes)

class newExpression : public Operator, public LibraryFunction
{
    size_t maxParameters() const { return 6; }
    size_t minParameters() const { return 0; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const size_t n = ast->num_atoms - 1;
        UniqueExpression type;
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
            case 6: type.reset(new likely_expression(Builder::type(Builder::validFloatType(likely_type_native))));
            case 5: channels = Builder::one();
            case 4: columns  = Builder::one();
            case 3: rows     = Builder::one();
            case 2: frames   = Builder::one();
            case 1: data     = Builder::nullData();
            default:           break;
        }

        return new likely_expression(createCall(builder, *type, channels, columns, rows, frames, data), likely_type_void);
    }

    void *symbol() const { return (void*) likely_new; }

public:
    static CallInst *createCall(Builder &builder, Value *type, Value *channels, Value *columns, Value *rows, Value *frames, Value *data)
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> params;
            params.push_back(NativeInt); // type
            params.push_back(NativeInt); // channels
            params.push_back(NativeInt); // columns
            params.push_back(NativeInt); // rows
            params.push_back(NativeInt); // frames
            params.push_back(Type::getInt8PtrTy(C)); // data
            functionType = FunctionType::get(T::Void, params, false);
        }

        Function *likelyNew = builder.module()->getFunction("likely_new");
        if (!likelyNew) {
            likelyNew = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_new", builder.module());
            likelyNew->setCallingConv(CallingConv::C);
            likelyNew->setDoesNotAlias(0);
            likelyNew->setDoesNotAlias(6);
            likelyNew->setDoesNotCapture(6);
        }

        vector<Value*> args;
        args.push_back(type);
        args.push_back(channels);
        args.push_back(columns);
        args.push_back(rows);
        args.push_back(frames);
        args.push_back(data);
        return builder.CreateCall(likelyNew, args);
    }
};
LIKELY_REGISTER(new)

class scalarExpression : public UnaryOperator, public LibraryFunction
{
    likely_expression *evaluateUnary(Builder &builder, likely_const_ast arg) const
    {
        likely_expression *argExpr = builder.expression(arg);
        if (!argExpr)
            return NULL;

        if (argExpr->isNull() || (argExpr->value()->getType() == T::Void))
            return argExpr;

        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> params;
            params.push_back(Type::getDoubleTy(C));
            params.push_back(NativeInt);
            functionType = FunctionType::get(T::Void, params, false);
        }
        Function *likelyScalar = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_scalar", builder.module());
        likelyScalar->setCallingConv(CallingConv::C);
        likelyScalar->setDoesNotAlias(0);

        likely_expression result(builder.CreateCall2(likelyScalar, builder.cast(argExpr, likely_type_f64), Builder::type(argExpr->type())), argExpr->type());
        delete argExpr;
        return new likely_expression(result);
    }

    void *symbol() const { return (void*) likely_scalar; }
};
LIKELY_REGISTER(scalar)

class stringExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new likely_expression(createCall(builder, arg), likely_type_i8);
    }

    void *symbol() const { return (void*) likely_string; }

public:
    static CallInst *createCall(Builder &builder, Value *string)
    {
        static FunctionType *functionType = FunctionType::get(T::Void, Type::getInt8PtrTy(C), false);
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

class copyExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new likely_expression(createCall(builder, arg), arg);
    }

    void *symbol() const { return (void*) likely_copy; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(T::Void, T::Void, false);
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

class retainExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new likely_expression(createCall(builder, arg), arg);
    }

    void *symbol() const { return (void*) likely_retain; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(T::Void, T::Void, false);
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

class releaseExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new likely_expression(createCall(builder, arg), arg);
    }

    void *symbol() const { return (void*) likely_release; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(Type::getVoidTy(C), T::Void, false);
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

struct Lambda : public ScopedExpression, public LibraryFunction
{
    Lambda(Builder &builder, likely_const_ast ast)
        : ScopedExpression(builder, ast) {}

    likely_expression *generate(Builder &builder, vector<T> types, string name) const
    {
        size_t n;
        if (ast->is_list && (ast->num_atoms > 1))
            if (ast->atoms[1]->is_list) n = ast->atoms[1]->num_atoms;
            else                        n = 1;
        else                            n = 0;

        while (types.size() < n)
            types.push_back(T::get(likely_type_void));

        Function *tmpFunction = cast<Function>(builder.module()->getOrInsertFunction(name+"_tmp", FunctionType::get(Type::getVoidTy(C), T::toLLVM(types), false)));
        vector<likely_expression> tmpArgs = builder.getArgs(tmpFunction, types);
        BasicBlock *entry = BasicBlock::Create(C, "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        likely_expression *result = evaluateFunction(builder, tmpArgs);
        if (!result || result->isNull()) return result;

        builder.CreateRet(*result);
        likely_type return_type = result->type();

        Function *function = cast<Function>(builder.module()->getOrInsertFunction(name, FunctionType::get(result->value()->getType(), T::toLLVM(types), false)));
        vector<likely_expression> args = builder.getArgs(function, types);

        ValueToValueMapTy VMap;
        for (size_t i=0; i<args.size(); i++)
            VMap[tmpArgs[i]] = args[i];

        SmallVector<ReturnInst*, 1> returns;
        CloneFunctionInto(function, tmpFunction, VMap, false, returns);
        tmpFunction->eraseFromParent();
        delete result;
        return new likely_expression(function, return_type);
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
            args.push_back(likely_expression(arg.value(), arg.type()));
        }

        likely_env restored = builder.env;
        builder.env = new likely_environment(env);
        builder.env->resources = Builder::lookupResources(restored);
        likely_expression *result = evaluateFunction(builder, args);
        likely_release_env(builder.env);
        builder.env = restored;
        return result;
    }

    likely_expression *evaluateFunction(Builder &builder, const vector<likely_expression> &args) const
    {
        // Do dynamic dispatch if the type isn't fully specified
        bool dynamic = false;
        for (const likely_expression &arg : args)
            dynamic = dynamic || (arg.type() == likely_type_void);
        dynamic = dynamic || (args.size() < ast->atoms[1]->num_atoms);

        if (dynamic) {
            VTable *vTable = new VTable(builder, ast, args.size());
            builder.lookupResources()->expressions.push_back(vTable);

            static PointerType *vTableType = PointerType::getUnqual(StructType::create(C, "VTable"));
            static FunctionType *likelyDynamicType = NULL;
            if (likelyDynamicType == NULL) {
                vector<Type*> params;
                params.push_back(vTableType);
                params.push_back(T::Void);
                likelyDynamicType = FunctionType::get(T::Void, params, true);
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

            vector<Value*> dynamicArgs;
            dynamicArgs.push_back(ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(C, 8*sizeof(vTable)), uintptr_t(vTable)), vTableType));
            for (const likely_expression &arg : args)
                dynamicArgs.push_back(arg);
            dynamicArgs.push_back(Builder::nullMat());
            return new likely_expression(builder.CreateCall(likelyDynamic, dynamicArgs), likely_type_void);
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

        if (ast->atoms[1]->is_list) {
            for (size_t i=0; i<args.size(); i++)
                builder.undefine(ast->atoms[1]->atoms[args.size()-i-1]->atom);
        } else {
            builder.undefine(ast->atoms[1]->atom);
        }
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

class beginExpression : public Operator
{
    size_t minParameters() const { return 0; }
    size_t maxParameters() const { return std::numeric_limits<size_t>::max(); }

    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        for (size_t i=1; i<ast->num_atoms-1; i++) {
            TRY_EXPR(builder, ast->atoms[i], expr)
        }
        return builder.expression(ast->atoms[ast->num_atoms-1]);
    }
};
LIKELY_REGISTER_EXPRESSION(begin, "{")

struct Label : public likely_expression
{
    BasicBlock *basicBlock;

public:
    Label(BasicBlock *basicBlock) : basicBlock(basicBlock) {}

private:
    Value *value() const
    {
        return basicBlock;
    }

    likely_type type() const
    {
        return likely_type_void;
    }

    likely_expression *evaluate(Builder &builder, likely_const_ast) const
    {
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
        builder.CreateCondBr(Cond, True, hasElse ? False : End);

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
            PHINode *phi = builder.CreatePHI(t.value()->getType(), 2);
            phi->addIncoming(t, True);
            if (hasElse) phi->addIncoming(f, False);
            return new likely_expression(phi, t);
        } else {
            builder.SetInsertPoint(End);
            return new likely_expression(NULL, likely_type_void);
        }
    }
};
LIKELY_REGISTER_EXPRESSION(if, "?")

struct Kernel : public Lambda
{
    Kernel(Builder &builder, likely_const_ast ast)
        : Lambda(builder, ast) {}

private:
    class kernelArgument : public Operator
    {
        likely_expression matrix;
        likely_type kernel;
        MDNode *node;

    public:
        kernelArgument(const likely_expression &matrix, likely_type kernel, MDNode *node)
            : matrix(matrix), kernel(kernel), node(node) {}

    private:
        size_t maxParameters() const { return 0; }
        likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
        {
            if (ast->is_list)
                return error(ast, "kernel operator does not take arguments");

            Value *i;
            if (((matrix ^ kernel) & likely_type_multi_dimension) == 0) {
                // This matrix has the same dimensionality as the kernel
                i = *builder.lookup("i");
            } else {
                Value *columnStep, *rowStep, *frameStep;
                builder.steps(&matrix, &columnStep, &rowStep, &frameStep);
                i = Builder::zero();
                if (likely_multi_channel(matrix)) i = *builder.lookup("c");
                if (likely_multi_column (matrix)) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("x"), columnStep), i);
                if (likely_multi_row    (matrix)) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("y"), rowStep   ), i);
                if (likely_multi_frame  (matrix)) i = builder.CreateAdd(builder.CreateMul(*builder.lookup("t"), frameStep ), i);
            }

            LoadInst *load = builder.CreateLoad(builder.CreateGEP(builder.data(&matrix), i));
            load->setMetadata("llvm.mem.parallel_loop_access", node);
            return new likely_expression(load, matrix);
        }
    };

    struct kernelAxis : public likely_expression
    {
        string name;
        Value *start, *stop;
        kernelAxis *parent, *child;
        MDNode *node;
        BasicBlock *loop, *exit;
        PHINode *index;
        Value *offset;
        BranchInst *latch = NULL;
        mutable bool referenced = false;

        kernelAxis(Builder &builder, const string &name, Value *start, Value *stop, Value *step, kernelAxis *parent)
            : name(name), start(start), stop(stop), parent(parent), child(NULL)
        {
            { // Create self-referencing loop node
                vector<Value*> metadata;
                MDNode *tmp = MDNode::getTemporary(C, metadata);
                metadata.push_back(tmp);
                node = MDNode::get(C, metadata);
                tmp->replaceAllUsesWith(node);
                MDNode::deleteTemporary(tmp);
            }

            // Loops assume at least one iteration
            BasicBlock *entry = builder.GetInsertBlock();
            loop = BasicBlock::Create(C, "loop_" + name, entry->getParent());
            builder.CreateBr(loop);
            builder.SetInsertPoint(loop);
            index = builder.CreatePHI(NativeInt, 2, name);
            index->addIncoming(start, entry);

            if (parent)
                parent->child = this;
            offset = builder.CreateAdd(parent ? parent->offset : Builder::zero(), builder.CreateMul(step, index), name + "_offset");
        }

        void close(Builder &builder)
        {
            Value *increment = builder.CreateAdd(index, builder.one(), name + "_increment");
            exit = BasicBlock::Create(C, name + "_exit", loop->getParent());
            latch = builder.CreateCondBr(builder.CreateICmpEQ(increment, stop, name + "_test"), exit, loop);
            latch->setMetadata("llvm.loop", node);
            index->addIncoming(increment, builder.GetInsertBlock());
            builder.SetInsertPoint(exit);
            if (parent) parent->close(builder);
        }

        void tryCollapse()
        {
            if (parent || referenced)
                return;

            while (child && !child->referenced) {
                // Collapse the child loop into us
                /*
                child->offset->replaceAllUsesWith(index);
                child->latch->setCondition(ConstantInt::getTrue(C));
                DeleteDeadPHIs(child->loop);
                MergeBlockIntoPredecessor(child->loop);
                MergeBlockIntoPredecessor(child->exit);
                */
                child = child->child;
            }
        }

        Value *value() const
        {
            referenced = true;
            return index;
        }

        likely_type type() const
        {
            return likely_type_native;
        }
    };

    likely_expression *evaluateLambda(Builder &builder, const vector<likely_expression> &srcs) const
    {
        vector<T> types;
        for (const likely_expression &src : srcs)
            types.push_back(T(src.value()->getType(), src.type()));

        vector<pair<likely_const_ast,likely_const_ast>> pairs;
        if (ast->num_atoms == 4)
            getPairs(ast->atoms[3], pairs);

        likely_type kernelType = likely_type_void;
        if (!srcs.empty())
            likely_set_execution(&kernelType, likely_execution(srcs.front()));

        for (const auto &pair : pairs)
            if (!strcmp("type", pair.first->atom) && !pair.second->is_list)
                kernelType |= likely_type_field_from_string(pair.second->atom, NULL);

        BasicBlock *entry = builder.GetInsertBlock();
        Value *dstChannels = getDimensions(builder, pairs, "channels", srcs, &kernelType);
        Value *dstColumns  = getDimensions(builder, pairs, "columns" , srcs, &kernelType);
        Value *dstRows     = getDimensions(builder, pairs, "rows"    , srcs, &kernelType);
        Value *dstFrames   = getDimensions(builder, pairs, "frames"  , srcs, &kernelType);

        Function *thunk;
        {
            { // declare thunk
                vector<Type*> params = T::toLLVM(types);
                params.push_back(T::Void);
                params.push_back(NativeInt);
                params.push_back(NativeInt);
                thunk = ::cast<Function>(builder.module()->getOrInsertFunction(getUniqueName("thunk"), FunctionType::get(Type::getVoidTy(C), params, false)));
                thunk->addFnAttr(Attribute::NoUnwind);
                thunk->setCallingConv(CallingConv::C);
                for (int i=1; i<int(types.size()+2); i++) {
                    thunk->setDoesNotAlias(i);
                    thunk->setDoesNotCapture(i);
                }
            }

            builder.SetInsertPoint(BasicBlock::Create(C, "entry", thunk));
            vector<likely_expression> srcs = builder.getArgs(thunk, types);
            likely_expression stop = srcs.back(); srcs.pop_back();
            stop.setType(likely_type_native);
            stop.value()->setName("stop");
            likely_expression start = srcs.back(); srcs.pop_back();
            start.setType(likely_type_native);
            start.value()->setName("start");
            likely_expression dst = srcs.back(); srcs.pop_back();
            dst.value()->setName("dst");
            dst.setType(kernelType);

            Value *channelStep = builder.one(), *columnStep, *rowStep, *frameStep;
            builder.steps(&dst, &columnStep, &rowStep, &frameStep);

            vector<kernelAxis*> axis;
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

                if (multiElement || ((axis_index == 3) && axis.empty())) {
                    if (axis.empty()) axis.push_back(new kernelAxis(builder, name, start, stop, step, NULL));
                    else              axis.push_back(new kernelAxis(builder, name, builder.zero(), elements, step, axis.back()));
                    builder.define(name.c_str(), axis.back()); // takes ownership of axis
                } else {
                    builder.define(name.c_str(), new likely_expression(builder.zero(), likely_type_native));
                }
            }
            builder.define("i", new likely_expression(axis.back()->offset, likely_type_native));

            const likely_const_ast args = ast->atoms[1];
            if (args->is_list) {
                assert(srcs.size() == args->num_atoms);
                for (size_t j=0; j<args->num_atoms; j++)
                    builder.define(args->atoms[j]->atom, new kernelArgument(srcs[j], dst, axis.back()->node));
            } else {
                assert(srcs.size() == 1);
                builder.define(args->atom, new kernelArgument(srcs[0], dst, axis.back()->node));
            }

            UniqueExpression result(builder.expression(ast->atoms[2]));
            likely_set_data(&kernelType, likely_data(result));
            dst.setType(kernelType);
            StoreInst *store = builder.CreateStore(result, builder.CreateGEP(builder.data(&dst), axis.back()->offset));
            store->setMetadata("llvm.mem.parallel_loop_access", axis.back()->node);

            axis.back()->close(builder); // Closes all axis
            axis.front()->tryCollapse();
            axis.clear();

            if (args->is_list) {
                for (size_t j=0; j<args->num_atoms; j++)
                    builder.undefine(args->atoms[args->num_atoms-j-1]->atom);
            } else {
                builder.undefine(args->atom);
            }
            builder.undefine("i");
            builder.undefine("c");
            builder.undefine("x");
            builder.undefine("y");
            builder.undefine("t");
            builder.CreateRetVoid();
        }

        builder.SetInsertPoint(entry);
        Value *dst = newExpression::createCall(builder, Builder::type(kernelType), dstChannels, dstColumns, dstRows, dstFrames, Builder::nullData());
        Value *kernelSize = likely_multi_frame (kernelType) ? dstFrames  :
                            likely_multi_row   (kernelType) ? dstRows    :
                            likely_multi_column(kernelType) ? dstColumns :
                                                              dstChannels;

        if (likely_parallel(kernelType)) {
            vector<Type*> likelyForkParameters;
            likelyForkParameters.push_back(thunk->getType());
            likelyForkParameters.push_back(Type::getInt8Ty(C));
            likelyForkParameters.push_back(NativeInt);
            likelyForkParameters.push_back(T::Void);
            Type *likelyForkReturn = Type::getVoidTy(C);
            FunctionType *likelyForkType = FunctionType::get(likelyForkReturn, likelyForkParameters, true);

            Function *likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "likely_fork", builder.module());
            likelyFork->setCallingConv(CallingConv::C);
            likelyFork->setDoesNotCapture(4);
            likelyFork->setDoesNotAlias(4);

            vector<Value*> likelyForkArgs;
            likelyForkArgs.push_back(builder.module()->getFunction(thunk->getName()));
            likelyForkArgs.push_back(Builder::constant(uint64_t(srcs.size()), likely_type_u8));
            likelyForkArgs.push_back(kernelSize);
            for (const likely_expression &src : srcs)
                likelyForkArgs.push_back(builder.CreatePointerCast(src, T::Void));
            likelyForkArgs.push_back(dst);
            builder.CreateCall(likelyFork, likelyForkArgs);
        } else {
            vector<Value*> thunkArgs;
            for (const likely_expression &src : srcs)
                thunkArgs.push_back(src);
            thunkArgs.push_back(dst);
            thunkArgs.push_back(Builder::zero());
            thunkArgs.push_back(kernelSize);
            builder.CreateCall(thunk, thunkArgs);
        }

        return new likely_expression(dst, kernelType);
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
                result = builder.cast(unique_ptr<likely_expression>(builder.expression(pair.second)).get(), likely_type_native);
                break;
            }

        // Use default dimensionality
        if (result == NULL) {
            if (srcs.empty()) {
                result = Builder::constant(1);
            } else {
                if      (!strcmp(axis, "channels")) result = builder.channels(&srcs[0]);
                else if (!strcmp(axis, "columns"))  result = builder.columns (&srcs[0]);
                else if (!strcmp(axis, "rows"))     result = builder.rows    (&srcs[0]);
                else                                result = builder.frames  (&srcs[0]);
            }
        }

        const bool multiElement = (!LLVM_VALUE_IS_INT(result)) || (LLVM_VALUE_TO_INT(result) > 1);
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

class exportExpression : public Operator
{
    size_t maxParameters() const { return 3; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[1], expr);
        Lambda *lambda = static_cast<Lambda*>(expr.get());

        if (ast->atoms[2]->is_list)
            return error(ast->atoms[2], "export expected an atom name");
        const char *name = ast->atoms[2]->atom;

        vector<T> types;
        if (ast->atoms[3]->is_list) {
            for (size_t i=0; i<ast->atoms[3]->num_atoms; i++) {
                if (ast->atoms[3]->atoms[i]->is_list)
                    return error(ast->atoms[2], "export expected an atom name");
                types.push_back(T::get(likely_type_from_string(ast->atoms[3]->atoms[i]->atom)));
            }
        } else {
            types.push_back(T::get(likely_type_from_string(ast->atoms[3]->atom)));
        }

        lambda->generate(builder, types, name);
        return NULL;
    }
};
LIKELY_REGISTER(export)

JITFunction::JITFunction(likely_const_ast ast, likely_env parent, const vector<likely_type> &type)
    : likely_resources(true), type(type)
{
    likely_assert(ast->is_list && (ast->num_atoms > 0) && !ast->atoms[0]->is_list &&
                  (!strcmp(ast->atoms[0]->atom, "->") || !strcmp(ast->atoms[0]->atom, "=>")),
                  "expected a lambda expression");
    likely_env env = new likely_environment(parent);
    env->resources = this;
    Builder builder(env);
    unique_ptr<likely_expression> result(builder.expression(ast));

    vector<T> types;
    for (likely_type t : type)
        types.push_back(T::get(t));
    UniqueExpression expr(static_cast<Lambda*>(result.get())->generate(builder, types, getUniqueName("jit")));
    error = !expr.get();
    likely_release_env(env);

    if (!error && !expr.isNull()) {
        string error;
        EngineBuilder engineBuilder(module);
        engineBuilder.setEngineKind(EngineKind::JIT)
                     .setErrorStr(&error)
                     .setUseMCJIT(true);
        EE = engineBuilder.create(getTargetMachine(true));
        likely_assert(EE != NULL, "failed to create execution engine with error: %s", error.c_str());

        Function *F = dyn_cast<Function>(expr.value());
        optimize();
        EE->finalizeObject();
        function = EE->getPointerToFunction(F);
    }
}

struct OfflineEnvironment : public likely_environment
{
    OfflineEnvironment(const string &fileName, bool native)
        : likely_environment(RootEnvironment)
    {
        resources = new OfflineResources(fileName, native);
    }

    OfflineEnvironment(OfflineEnvironment *parent)
        : likely_environment(parent)
    {}

    ~OfflineEnvironment()
    {
        delete resources;
    }

private:
    likely_env evaluate(likely_const_ast ast)
    {
        likely_env env = new OfflineEnvironment(this);
        Builder builder(env);
        UniqueExpression e(builder.expression(ast));
        env->result = e.get() ? likely_void() : NULL;
        return env;
    }
};

#ifdef LIKELY_IO
#include "likely/io.h"

class printExpression : public Operator, public LibraryFunction
{
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }
    size_t minParameters() const { return 0; }
    likely_expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        static FunctionType *functionType = FunctionType::get(T::Void, T::Void, true);
        Function *likelyPrint = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_print", builder.module());
        likelyPrint->setCallingConv(CallingConv::C);
        likelyPrint->setDoesNotAlias(0);
        likelyPrint->setDoesNotAlias(1);
        likelyPrint->setDoesNotCapture(1);

        vector<Value*> rawArgs;
        for (size_t i=1; i<ast->num_atoms; i++) {
            TRY_EXPR(builder, ast->atoms[i], arg);
            rawArgs.push_back(arg);
        }
        rawArgs.push_back(builder.nullMat());

        vector<Value*> matArgs;
        for (Value *rawArg : rawArgs)
            if (rawArg->getType() == T::Void) {
                matArgs.push_back(rawArg);
            } else {
                matArgs.push_back(stringExpression::createCall(builder, rawArg));
            }

        Value *result = builder.CreateCall(likelyPrint, matArgs);

        for (size_t i=0; i<rawArgs.size(); i++)
            if (rawArgs[i] != matArgs[i])
                releaseExpression::createCall(builder, matArgs[i]);

        return new likely_expression(result, likely_type_i8);
    }

    void *symbol() const { return (void*) likely_print; }
};
LIKELY_REGISTER(print)

class readExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(T::Void, Type::getInt8PtrTy(C), false);
        Function *likelyRead = builder.module()->getFunction("likely_read");
        if (!likelyRead) {
            likelyRead = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_read", builder.module());
            likelyRead->setCallingConv(CallingConv::C);
            likelyRead->setDoesNotAlias(0);
            likelyRead->setDoesNotAlias(1);
            likelyRead->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyRead, arg), likely_type_void);
    }
    void *symbol() const { return (void*) likely_read; }
};
LIKELY_REGISTER(read)

class writeExpression : public SimpleBinaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &arg1, const UniqueExpression &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> likelyWriteParameters;
            likelyWriteParameters.push_back(T::Void);
            likelyWriteParameters.push_back(Type::getInt8PtrTy(C));
            functionType = FunctionType::get(T::Void, likelyWriteParameters, false);
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
        vector<Value*> likelyWriteArguments;
        likelyWriteArguments.push_back(arg1);
        likelyWriteArguments.push_back(arg2);
        return new likely_expression(builder.CreateCall(likelyWrite, likelyWriteArguments), likely_type_void);
    }
    void *symbol() const { return (void*) likely_write; }
};
LIKELY_REGISTER(write)

class decodeExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(T::Void, T::Void, false);
        Function *likelyDecode = builder.module()->getFunction("likely_decode");
        if (!likelyDecode) {
            likelyDecode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_decode", builder.module());
            likelyDecode->setCallingConv(CallingConv::C);
            likelyDecode->setDoesNotAlias(0);
            likelyDecode->setDoesNotAlias(1);
            likelyDecode->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyDecode, arg), likely_type_void);
    }
    void *symbol() const { return (void*) likely_decode; }
};
LIKELY_REGISTER(decode)

class encodeExpression : public SimpleBinaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &arg1, const UniqueExpression &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> parameters;
            parameters.push_back(T::Void);
            parameters.push_back(Type::getInt8PtrTy(C));
            functionType = FunctionType::get(T::Void, parameters, false);
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
        vector<Value*> likelyEncodeArguments;
        likelyEncodeArguments.push_back(arg1);
        likelyEncodeArguments.push_back(arg2);
        return new likely_expression(builder.CreateCall(likelyEncode, likelyEncodeArguments), likely_type_void);
    }
    void *symbol() const { return (void*) likely_encode; }
};
LIKELY_REGISTER(encode)

class renderExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> parameters;
            parameters.push_back(T::Void);
            parameters.push_back(Type::getDoublePtrTy(C));
            parameters.push_back(Type::getDoublePtrTy(C));
            functionType = FunctionType::get(T::Void, parameters, false);
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
        vector<Value*> likelyRenderArguments;
        likelyRenderArguments.push_back(arg);
        likelyRenderArguments.push_back(ConstantPointerNull::get(Type::getDoublePtrTy(C)));
        likelyRenderArguments.push_back(ConstantPointerNull::get(Type::getDoublePtrTy(C)));
        return new likely_expression(builder.CreateCall(likelyRender, likelyRenderArguments), likely_type_void);
    }
    void *symbol() const { return (void*) likely_render; }
};
LIKELY_REGISTER(render)

class showExpression : public SimpleUnaryOperator, public LibraryFunction
{
    likely_expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> parameters;
            parameters.push_back(T::Void);
            parameters.push_back(Type::getInt8PtrTy(C));
            FunctionType::get(Type::getVoidTy(C), parameters, false);
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
        return new likely_expression(builder.CreateCall2(likelyShow, arg, ConstantPointerNull::get(Type::getInt8PtrTy(C))), likely_type_void);
    }
    void *symbol() const { return (void*) likely_decode; }
};
LIKELY_REGISTER(show)
#endif // LIKELY_IO

} // namespace (anonymous)

likely_env likely_environment::evaluate(likely_const_ast ast)
{
    likely_env env = new likely_environment(this);
    if (ast->is_list && (ast->num_atoms > 0) && !strcmp(ast->atoms[0]->atom, "=")) {
        // Shortcut for global variable definitions
        delete Builder(env).expression(ast);
        env->result = likely_void();
    } else {
        likely_const_ast expr = likely_ast_from_string("() -> (scalar <ast>)", false);
        expr->atoms[2]->atoms[1] = likely_retain_ast(ast);
        JITFunction resources(expr, env, vector<likely_type>());
        likely_release_ast(expr);
        if      (resources.function) env->result = reinterpret_cast<likely_mat(*)(void)>(resources.function)();
        else if (!resources.error)   env->result = likely_void();
        else                         env->result = NULL;
    }
    return env;
}

likely_env likely_new_jit()
{
    return new likely_environment(RootEnvironment);
}

likely_env likely_new_offline(const char *file_name, bool native)
{
    return new OfflineEnvironment(file_name, native);
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
    delete   env->value;
    likely_release(env->result);
    delete env;
}

likely_mat likely_dynamic(struct VTable *vTable, likely_const_mat m, ...)
{
    vector<likely_const_mat> mv(vTable->n);
    va_list ap;
    va_start(ap, m);
    for (size_t i=0; i<vTable->n; i++) {
        mv[i] = m;
        m = va_arg(ap, likely_const_mat);
    }
    va_end(ap);
    assert(m == NULL);

    void *function = NULL;
    for (size_t i=0; i<vTable->functions.size(); i++) {
        const unique_ptr<JITFunction> &resources = vTable->functions[i];
        for (likely_arity j=0; j<vTable->n; j++)
            if (mv[j]->type != resources->type[j])
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
        for (size_t i=0; i<vTable->n; i++)
            types.push_back(mv[i]->type);
        vTable->functions.push_back(unique_ptr<JITFunction>(new JITFunction(vTable->ast, vTable->env, types)));
        function = vTable->functions.back()->function;
    }

    likely_mat dst;
    switch (vTable->n) {
      case 0: dst = reinterpret_cast<likely_function_0>(function)(); break;
      case 1: dst = reinterpret_cast<likely_function_1>(function)(mv[0]); break;
      case 2: dst = reinterpret_cast<likely_function_2>(function)(mv[0], mv[1]); break;
      case 3: dst = reinterpret_cast<likely_function_3>(function)(mv[0], mv[1], mv[2]); break;
      default: dst = NULL; likely_assert(false, "likely_dynamic invalid arity: %d", vTable->n);
    }

    return dst;
}

static map<likely_function, pair<JITFunction*,int>> ResourcesLUT;

likely_function likely_compile(likely_const_ast ast, likely_env env, likely_type type, ...)
{
    if (!ast || !env) return NULL;
    vector<likely_type> types;
    va_list ap;
    va_start(ap, type);
    while (type != likely_type_void) {
        types.push_back(type);
        type = va_arg(ap, likely_type);
    }
    va_end(ap);
    JITFunction *r = new JITFunction(ast, env, types);
    likely_function f = reinterpret_cast<likely_function>(r->function);
    if (f) ResourcesLUT[f] = pair<JITFunction*,int>(r, 1);
    else   delete r;
    return f;
}

likely_function likely_retain_function(likely_function function)
{
    if (function) ResourcesLUT[function].second++;
    return function;
}

void likely_release_function(likely_function function)
{
    if (!function) return;
    pair<JITFunction*,int> &df = ResourcesLUT[function];
    if (--df.second) return;
    ResourcesLUT.erase(function);
    delete df.first;
}

likely_mat likely_eval(likely_const_ast ast, likely_env *env)
{
    if (!ast || !env || !*env) return NULL;
    likely_env new_env = (*env)->evaluate(ast);
    likely_mat result = likely_retain(new_env->result);
    likely_release_env(*env);
    *env = new_env;
    return result;
}

bool likely_repl(const char *source, bool GFM, likely_env *env, likely_env prev)
{
    likely_const_ast asts = likely_asts_from_string(source, GFM);
    if (!asts)
        return false;

    bool owns_env = !env;
    if (owns_env) {
        env = (likely_env*) alloca(sizeof(likely_env*));
        *env = NULL;
    }
    if (*env == NULL)
        *env = likely_new_jit();

    int prevDepth = 0;
    {
        likely_env tmp = prev;
        while (tmp && (tmp->parent != (*env)->parent)) {
            tmp = tmp->parent;
            prevDepth++;
        }
        if (!tmp)
            prevDepth = -1;
    }

    bool success = true;
    for (size_t i=0; i<asts->num_atoms; i++) {
        likely_const_ast ast = asts->atoms[i];
        if (likely_const_mat m = likely_eval(ast, env)) {
            if (likely_elements(m) > 0)
                likely_show(m, ast->is_list ? NULL : ast->atom);
            likely_release(m);
        } else {
            success = false;
            break;
        }
    }

    if (owns_env)
        likely_release_env(*env);
    likely_release_ast(asts);
    return success;
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

        const bool success = likely_repl(source.c_str(), true, &builder.env, NULL);
        if (success) return new likely_expression();
        else         return NULL;
    }
};
LIKELY_REGISTER(import)

} // namespace (anonymous)
