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
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/ToolOutputFile.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Vectorize.h>
#include <cstdarg>
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
struct Expression
{
    virtual ~Expression() {}
    virtual Value *value() const = 0;
    virtual likely_type type() const = 0;
    virtual Expression *evaluate(Builder &builder, likely_const_ast ast) const = 0;

    operator Value*() const { return value(); }
    operator likely_type() const { return type(); }

    Value *take()
    {
        Value *result = value();
        delete this; // With great power comes great responsibility
        return result;
    }

    static Expression *error(likely_const_ast ast, const char *message)
    {
        likely_throw(ast, message);
        return NULL;
    }
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

struct UniqueExpression : public Expression, public unique_ptr<Expression>
{
    UniqueExpression(Expression *e = NULL)
        : unique_ptr<Expression>(e) {}
    bool isNull() const { return !get(); }
    Value* value() const { return get()->value(); }
    likely_type type() const { return get()->type(); }
    Expression *evaluate(Builder &builder, likely_const_ast ast) const
        { return get()->evaluate(builder, ast); }
};

#define TRY_EXPR(BUILDER, AST, EXPR)                    \
const UniqueExpression EXPR((BUILDER).expression(AST)); \
if (EXPR.isNull()) return NULL;                         \

static string getUniqueName(const string &prefix)
{
    static map<string, int> uidLUT;
    stringstream stream;
    stream << "likely_" << prefix << "_" << uidLUT[prefix]++;
    return stream.str();
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

struct Resources
{
    TargetMachine *TM = NULL;
    Module *module;
    vector<Expression*> expressions;

    Resources(bool native)
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

        if (native) {
            module->setTargetTriple(sys::getProcessTriple());

            static TargetMachine *nativeTM = NULL;
            if (!nativeTM) {
                string error;
                EngineBuilder engineBuilder(module);
                engineBuilder.setMCPU(sys::getHostCPUName())
                             .setCodeModel(CodeModel::Default)
                             .setErrorStr(&error);
                nativeTM = engineBuilder.selectTarget();
                likely_assert(nativeTM != NULL, "failed to select target machine with error: %s", error.c_str());
            }
            TM = nativeTM;
        }
    }

    void optimize()
    {
        if (!TM)
            return;

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

    ~Resources()
    {
        for (Expression *e : expressions)
            delete e;
        delete module;
    }
};

class JITResources : public Resources
{
    ExecutionEngine *EE = NULL;

public:
    void *function = NULL;
    const vector<likely_type> type;

    JITResources(likely_const_ast ast, likely_env env, const vector<likely_type> &type);

    ~JITResources()
    {
        module = NULL;
        delete EE; // owns module
    }
};

class OfflineResources : public Resources
{
    const string fileName;

public:
    OfflineResources(const string &fileName, bool native)
        : Resources(native), fileName(fileName)
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
            likely_assert(TM != NULL, "missing target machine for object file output");
            TM->addPassesToEmitFile(pm, fos, extension == "s" ? TargetMachine::CGFT_AssemblyFile : TargetMachine::CGFT_ObjectFile);
            pm.run(*module);
        }

        likely_assert(errorInfo.empty(), "failed to write to: %s with error: %s", fileName.c_str(), errorInfo.c_str());
        output.keep();
    }
};

} // namespace (anonymous)

struct likely_environment : public map<string,stack<shared_ptr<Expression>>>
{
    static map<string,stack<shared_ptr<Expression>>> defaultExprs;
    mutable int ref_count = 1;
    likely_environment(const map<string,stack<shared_ptr<Expression>>> &exprs = defaultExprs)
        : map<string,stack<shared_ptr<Expression>>>(exprs) {}
    virtual ~likely_environment() {}
    virtual likely_mat evaluate(likely_const_ast ast)
    {
        likely_const_ast expr = likely_ast_from_string("() -> (scalar <ast>)", false);
        expr->atoms[2]->atoms[1] = likely_retain_ast(ast);
        JITResources resources(expr, this, vector<likely_type>());
        likely_release_ast(expr);
        if (resources.function) return reinterpret_cast<likely_mat(*)(void)>(resources.function)();
        else                    return NULL;
    }
};
map<string,stack<shared_ptr<Expression>>> likely_environment::defaultExprs;

namespace {

struct Immediate : public Expression
{
    Value *value_;
    likely_type type_;

    Immediate() : value_(NULL), type_(likely_type_void) {}
    Immediate(Value *value, likely_type type)
        : value_(value), type_(type) {}
    bool isNull() { return !value_; }

private:
    Value *value() const { return value_; }
    likely_type type() const { return type_; }
    Expression *evaluate(Builder &, likely_const_ast) const
    {
        return new Immediate(value(), type());
    }
};

struct Builder : public IRBuilder<>
{
    likely_env env;
    Resources *resources;
    map<string, UniqueExpression> locals;

    Builder(Resources *resources, likely_env env)
        : IRBuilder<>(C), env(env), resources(resources)
    {}

    static Immediate constant(uint64_t value, likely_type type = likely_type_native)
    {
        const int depth = likely_depth(type);
        return Immediate(Constant::getIntegerValue(Type::getIntNTy(C, depth), APInt(depth, value)), type);
    }

    static Immediate constant(int value, likely_type type)
    {
        return constant(uint64_t(value), type);
    }

    static Immediate constant(double value, likely_type type)
    {
        const int depth = likely_depth(type);
        if (likely_floating(type)) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return Immediate(ConstantFP::get(Type::getDoubleTy(C), value), type);
            else if (depth == 32) return Immediate(ConstantFP::get(Type::getFloatTy(C), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return Immediate(NULL, likely_type_void); }
        } else {
            return constant(uint64_t(value), type);
        }
    }

    static Immediate zero(likely_type type = likely_type_native) { return constant(0, type); }
    static Immediate one (likely_type type = likely_type_native) { return constant(1, type); }
    static Immediate intMax(likely_type type) { const int bits = likely_depth(type); return constant((1 << (bits - (likely_signed(type) ? 1 : 0)))-1, bits); }
    static Immediate intMin(likely_type type) { const int bits = likely_depth(type); return constant(likely_signed(type) ? (1 << (bits - 1)) : 0, bits); }
    static Immediate type(likely_type type) { return constant(uint64_t(type), likely_type_type); }
    static Immediate nullMat() { return Immediate(ConstantPointerNull::get(T::Void), likely_type_void); }
    static Immediate nullData() { return Immediate(ConstantPointerNull::get(Type::getInt8PtrTy(C)), likely_type_native); }

    Immediate channels(const Expression *matrix) { return likely_multi_channel(*matrix) ? Immediate(CreateLoad(CreateStructGEP(*matrix, 2), "channels"), likely_type_native) : one(); }
    Immediate columns (const Expression *matrix) { return likely_multi_column (*matrix) ? Immediate(CreateLoad(CreateStructGEP(*matrix, 3), "columns" ), likely_type_native) : one(); }
    Immediate rows    (const Expression *matrix) { return likely_multi_row    (*matrix) ? Immediate(CreateLoad(CreateStructGEP(*matrix, 4), "rows"    ), likely_type_native) : one(); }
    Immediate frames  (const Expression *matrix) { return likely_multi_frame  (*matrix) ? Immediate(CreateLoad(CreateStructGEP(*matrix, 5), "frames"  ), likely_type_native) : one(); }
    Immediate data    (const Expression *matrix) { return Immediate(CreatePointerCast(CreateStructGEP(*matrix, 7), ty(*matrix, true)), likely_data(*matrix)); }

    void steps(const Expression *matrix, Value **columnStep, Value **rowStep, Value **frameStep)
    {
        *columnStep = channels(matrix);
        *rowStep    = CreateMul(columns(matrix), *columnStep, "y_step");
        *frameStep  = CreateMul(rows(matrix), *rowStep, "t_step");
    }

    Immediate cast(const Expression *x, likely_type type)
    {
        if (likely_data(*x) == likely_data(type))
            return Immediate(*x, type);
        Type *dstType = ty(type);
        return Immediate(CreateCast(CastInst::getCastOpcode(*x, likely_signed(*x), dstType, likely_signed(type)), *x, dstType), type);
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

    vector<Immediate> getArgs(Function *function, const vector<T> &types)
    {
        vector<Immediate> result;
        Function::arg_iterator args = function->arg_begin();
        likely_arity n = 0;
        while (args != function->arg_end()) {
            Value *src = args++;
            stringstream name; name << "arg_" << int(n);
            src->setName(name.str());
            result.push_back(Immediate(src, n < types.size() ? types[n].likely : likely_type(likely_type_void)));
            n++;
        }
        return result;
    }

    void define(const string &name, Expression *e)
    {
        (*env)[name].push(shared_ptr<Expression>(e));
    }

    void define(const string &name, const Immediate &i)
    {
        define(name, new Immediate(i));
    }

    void define(const string &name, Value *value, likely_type type)
    {
        define(name, Immediate(value, type));
    }

    void undefine(const string &name)
    {
        (*env)[name].pop();
    }

    Expression *lookup(const string &name)
    {
        auto it = env->find(name);
        if (it != env->end()) return it->second.top().get();
        else                  return NULL;
    }

    likely_env snapshot() const { return new likely_environment(*env); }

    Expression *expression(likely_const_ast ast);
};

class Operator : public Expression
{
    Value *value() const
    {
        likely_assert(false, "Operator has no value!");
        return NULL;
    }

    likely_type type() const
    {
        likely_assert(false, "Operator has no type!");
        return likely_type_void;
    }

    Expression *evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (!ast->is_list && (minParameters() > 0))
            return error(ast, "operator expected arguments");
        const size_t args = ast->is_list ? ast->num_atoms - 1 : 0;
        if ((args < minParameters()) || (args > maxParameters()))
            return errorArgc(ast, "operator", args, minParameters(), maxParameters());
        return evaluateOperator(builder, ast);
    }

    virtual Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const = 0;
    virtual size_t maxParameters() const = 0;
    virtual size_t minParameters() const { return maxParameters(); }

protected:
    static Expression *errorArgc(likely_const_ast ast, const string &function, size_t args, size_t minParams, size_t maxParams)
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
        : env(builder.snapshot()), ast(likely_retain_ast(ast)) {}

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
    vector<JITResources*> functions;

    VTable(Builder &builder, likely_const_ast ast, size_t n)
        : ScopedExpression(builder, ast), n(n)
    {}

    ~VTable()
    {
        for (Resources *function : functions)
            delete function;
    }

    Expression *evaluateOperator(Builder &, likely_const_ast) const
    {
        return NULL;
    }
};

extern "C" LIKELY_EXPORT likely_mat likely_dynamic(struct VTable *vtable, likely_const_mat m, ...);

namespace {

class LibraryFunction
{
    virtual void *symbol() const = 0; // Idiom to ensure that the library symbol isn't stripped when optimizing executable size
};

template <class T>
struct RegisterExpression
{
    RegisterExpression(const string &symbol)
    {
        likely_environment::defaultExprs[symbol].push(shared_ptr<Expression>(new T()));
    }
};
#define LIKELY_REGISTER_EXPRESSION(EXP, SYM) static struct RegisterExpression<EXP##Expression> Register##EXP##Expression(SYM);
#define LIKELY_REGISTER(EXP) LIKELY_REGISTER_EXPRESSION(EXP, #EXP)

#define LIKELY_REGISTER_TYPE(TYPE)                        \
struct TYPE##Expression : public Immediate                \
{                                                         \
    TYPE##Expression()                                    \
        : Immediate(Builder::type(likely_type_##TYPE)) {} \
};                                                        \
LIKELY_REGISTER(TYPE)                                     \

LIKELY_REGISTER_TYPE(void)
LIKELY_REGISTER_TYPE(depth)
LIKELY_REGISTER_TYPE(signed)
LIKELY_REGISTER_TYPE(floating)
LIKELY_REGISTER_TYPE(data)
LIKELY_REGISTER_TYPE(parallel)
LIKELY_REGISTER_TYPE(heterogeneous)
LIKELY_REGISTER_TYPE(multi_channel)
LIKELY_REGISTER_TYPE(multi_column)
LIKELY_REGISTER_TYPE(multi_row)
LIKELY_REGISTER_TYPE(multi_frame)
LIKELY_REGISTER_TYPE(multi_dimension)
LIKELY_REGISTER_TYPE(saturation)

class UnaryOperator : public Operator
{
    size_t maxParameters() const { return 1; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        return evaluateUnary(builder, ast->atoms[1]);
    }
    virtual Expression *evaluateUnary(Builder &builder, likely_const_ast arg) const = 0;
};

class SimpleUnaryOperator : public UnaryOperator
{
    Expression *evaluateUnary(Builder &builder, likely_const_ast arg) const
    {
        TRY_EXPR(builder, arg, expr)
        return evaluateSimpleUnary(builder, expr);
    }
    virtual Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const = 0;
};

struct MatrixType : public SimpleUnaryOperator
{
    likely_type t;
    MatrixType(likely_type t)
        : t(t) {}

    Value *value() const { return Builder::type(t); }
    likely_type type() const { return likely_type_type; }

    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &x) const
    {
        return new Immediate(builder.cast(&x, t));
    }
};

Expression *Builder::expression(likely_const_ast ast)
{
    if (ast->is_list) {
        if (ast->num_atoms == 0)
            return Expression::error(ast, "Empty expression");
        likely_const_ast op = ast->atoms[0];
        if (!op->is_list)
            if (Expression *e = lookup(op->atom))
                return e->evaluate(*this, ast);
        TRY_EXPR(*this, op, e);
        return e.evaluate(*this, ast);
    }
    const string op = ast->atom;

    { // Is it a local variable?
        auto var = locals.find(op);
        if (var != locals.end())
            return var->second.evaluate(*this, ast);
    }

    if (Expression *e = lookup(op))
        return e->evaluate(*this, ast);

    if ((op.front() == '"') && (op.back() == '"'))
        return new Immediate(CreateGlobalStringPtr(op.substr(1, op.length()-2)), likely_type_u8);

    { // Is it a number?
        char *p;
        const double value = strtod(op.c_str(), &p);
        if (*p == 0)
            return new Immediate(constant(value, likely_type_from_value(value)));
    }

    likely_type type = likely_type_from_string(op.c_str());
    if (type != likely_type_void)
        return new MatrixType(type);

    return Expression::error(ast, "unrecognized literal");
}

#define LIKELY_REGISTER_FIELD(FIELD)                                                     \
class FIELD##Expression : public SimpleUnaryOperator                                     \
{                                                                                        \
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const \
    {                                                                                    \
        return new Immediate(builder.FIELD(&arg));                                       \
    }                                                                                    \
};                                                                                       \
LIKELY_REGISTER(FIELD)                                                                   \

LIKELY_REGISTER_FIELD(channels)
LIKELY_REGISTER_FIELD(columns)
LIKELY_REGISTER_FIELD(rows)
LIKELY_REGISTER_FIELD(frames)

class notExpression : public SimpleUnaryOperator
{
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new Immediate(builder.CreateXor(Builder::intMax(arg), arg.value()), arg);
    }
};
LIKELY_REGISTER_EXPRESSION(not, "~")

class typeExpression : public SimpleUnaryOperator
{
    Expression *evaluateSimpleUnary(Builder &, const UniqueExpression &arg) const
    {
        return new MatrixType(arg);
    }
};
LIKELY_REGISTER(type)

class UnaryMathOperator : public SimpleUnaryOperator
{
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &x) const
    {
        Immediate xc(builder.cast(&x, Builder::validFloatType(x)));
        vector<Type*> args;
        args.push_back(xc.value_->getType());
        return new Immediate(builder.CreateCall(Intrinsic::getDeclaration(builder.resources->module, id(), args), xc), xc);
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
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[1], expr1)
        TRY_EXPR(builder, ast->atoms[2], expr2)
        return evaluateSimpleBinary(builder, expr1, expr2);
    }
    virtual Expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &arg1, const UniqueExpression &arg2) const = 0;
};

class ArithmeticOperator : public SimpleBinaryOperator
{
    Expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &lhs, const UniqueExpression &rhs) const
    {
        likely_type type = likely_type_from_types(lhs, rhs);
        return evaluateArithmetic(builder, builder.cast(&lhs, type), builder.cast(&rhs, type));
    }
    virtual Expression *evaluateArithmetic(Builder &builder, const Immediate &lhs, const Immediate &rhs) const = 0;
};

class addExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Immediate &lhs, const Immediate &rhs) const
    {
        if (likely_floating(lhs)) {
            return new Immediate(builder.CreateFAdd(lhs, rhs), lhs);
        } else {
            if (likely_saturation(lhs)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.resources->module, likely_signed(lhs) ? Intrinsic::sadd_with_overflow : Intrinsic::uadd_with_overflow, lhs.value_->getType()), lhs, rhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, Builder::zero(lhs)), Builder::intMax(lhs), Builder::intMin(lhs)) : Builder::intMax(lhs);
                return new Immediate(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), lhs);
            } else {
                return new Immediate(builder.CreateAdd(lhs, rhs), lhs);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(add, "+")

class subtractExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Immediate &lhs, const Immediate &rhs) const
    {
        if (likely_floating(lhs)) {
            return new Immediate(builder.CreateFSub(lhs, rhs), lhs);
        } else {
            if (likely_saturation(lhs)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.resources->module, likely_signed(lhs) ? Intrinsic::ssub_with_overflow : Intrinsic::usub_with_overflow, lhs.value_->getType()), lhs, rhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateICmpSGE(lhs.value_, Builder::zero(lhs)), Builder::intMax(lhs), Builder::intMin(lhs)) : Builder::intMin(lhs);
                return new Immediate(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), lhs);
            } else {
                return new Immediate(builder.CreateSub(lhs, rhs), lhs);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(subtract, "-")

class multiplyExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Immediate &lhs, const Immediate &rhs) const
    {
        if (likely_floating(lhs)) {
            return new Immediate(builder.CreateFMul(lhs, rhs), lhs);
        } else {
            if (likely_saturation(lhs)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.resources->module, likely_signed(lhs) ? Intrinsic::smul_with_overflow : Intrinsic::umul_with_overflow, lhs.value_->getType()), lhs, rhs);
                Value *zero = Builder::zero(lhs);
                Value *overflowResult = likely_signed(lhs) ? builder.CreateSelect(builder.CreateXor(builder.CreateICmpSGE(lhs.value_, zero), builder.CreateICmpSGE(rhs.value_, zero)), Builder::intMin(lhs), Builder::intMax(lhs)) : Builder::intMax(lhs);
                return new Immediate(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), lhs);
            } else {
                return new Immediate(builder.CreateMul(lhs, rhs), lhs);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(multiply, "*")

class divideExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Immediate &n, const Immediate &d) const
    {
        if (likely_floating(n)) {
            return new Immediate(builder.CreateFDiv(n, d), n);
        } else {
            if (likely_signed(n)) {
                if (likely_saturation(n)) {
                    Value *safe_i = builder.CreateAdd(n, builder.CreateZExt(builder.CreateICmpNE(builder.CreateOr(builder.CreateAdd(d.value_, Builder::one(n)), builder.CreateAdd(n.value_, Builder::intMin(n))), Builder::zero(n)), n.value_->getType()));
                    return new Immediate(builder.CreateSDiv(safe_i, d), n);
                } else {
                    return new Immediate(builder.CreateSDiv(n, d), n);
                }
            } else {
                return new Immediate(builder.CreateUDiv(n, d), n);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(divide, "/")

class remExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Immediate &lhs, const Immediate &rhs) const
    {
        return new Immediate(likely_floating(lhs) ? builder.CreateFRem(lhs, rhs)
                                                  : (likely_signed(lhs) ? builder.CreateSRem(lhs, rhs)
                                                                        : builder.CreateURem(lhs, rhs)), lhs);
    }
};
LIKELY_REGISTER_EXPRESSION(rem, "%")

#define LIKELY_REGISTER_LOGIC(OP, SYM)                                                                 \
class OP##Expression : public ArithmeticOperator                                                       \
{                                                                                                      \
    Expression *evaluateArithmetic(Builder &builder, const Immediate &lhs, const Immediate &rhs) const \
    {                                                                                                  \
        return new Immediate(builder.Create##OP(lhs, rhs.value_), lhs);                                \
    }                                                                                                  \
};                                                                                                     \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                    \

LIKELY_REGISTER_LOGIC(And, "and")
LIKELY_REGISTER_LOGIC(Or, "or")
LIKELY_REGISTER_LOGIC(Xor, "xor")
LIKELY_REGISTER_LOGIC(Shl, "shl")
LIKELY_REGISTER_LOGIC(LShr, "lshr")
LIKELY_REGISTER_LOGIC(AShr, "ashr")

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                    \
class OP##Expression : public ArithmeticOperator                                                                               \
{                                                                                                                              \
    Expression *evaluateArithmetic(Builder &builder, const Immediate &lhs, const Immediate &rhs) const                         \
    {                                                                                                                          \
        return new Immediate(likely_floating(lhs) ? builder.CreateFCmpO##OP(lhs, rhs)                                          \
                                                  : (likely_signed(lhs) ? builder.CreateICmpS##OP(lhs, rhs)                    \
                                                                        : builder.CreateICmpU##OP(lhs, rhs)), likely_type_u1); \
    }                                                                                                                          \
};                                                                                                                             \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                            \

LIKELY_REGISTER_COMPARISON(LT, "<")
LIKELY_REGISTER_COMPARISON(LE, "<=")
LIKELY_REGISTER_COMPARISON(GT, ">")
LIKELY_REGISTER_COMPARISON(GE, ">=")

#define LIKELY_REGISTER_EQUALITY(OP, SYM)                                                              \
class OP##Expression : public ArithmeticOperator                                                       \
{                                                                                                      \
    Expression *evaluateArithmetic(Builder &builder, const Immediate &lhs, const Immediate &rhs) const \
    {                                                                                                  \
        return new Immediate(likely_floating(lhs) ? builder.CreateFCmpO##OP(lhs, rhs)                  \
                                                  : builder.CreateICmp##OP(lhs, rhs), likely_type_u1); \
    }                                                                                                  \
};                                                                                                     \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                    \

LIKELY_REGISTER_EQUALITY(EQ, "==")
LIKELY_REGISTER_EQUALITY(NE, "!=")

class BinaryMathOperator : public SimpleBinaryOperator
{
    Expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &x, const UniqueExpression &n) const
    {
        const likely_type type = nIsInteger() ? x.type() : likely_type_from_types(x, n);
        Immediate xc(builder.cast(&x, Builder::validFloatType(type)));
        Immediate nc(builder.cast(&n, nIsInteger() ? likely_type(likely_type_i32) : xc.type_));
        vector<Type*> args;
        args.push_back(xc.value_->getType());
        return new Immediate(builder.CreateCall2(Intrinsic::getDeclaration(builder.resources->module, id(), args), xc, nc), xc);
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

class SimpleTernaryOperator : public Operator
{
    size_t maxParameters() const { return 3; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[1], a)
        TRY_EXPR(builder, ast->atoms[2], b)
        TRY_EXPR(builder, ast->atoms[3], c)
        return evaluateSimpleTernary(builder, a, b, c);
    }
    virtual Expression *evaluateSimpleTernary(Builder &builder, const UniqueExpression &arg1, const UniqueExpression &arg2, const UniqueExpression &arg3) const = 0;
};

class fmaExpression : public SimpleTernaryOperator
{
    Expression *evaluateSimpleTernary(Builder &builder, const UniqueExpression &a, const UniqueExpression &b, const UniqueExpression &c) const
    {
        const likely_type type = likely_type_from_types(likely_type_from_types(a, b), c);
        Immediate ac(builder.cast(&a, Builder::validFloatType(type)));
        Immediate bc(builder.cast(&b, ac));
        Immediate cc(builder.cast(&c, ac));
        vector<Type*> args;
        args.push_back(ac.value_->getType());
        return new Immediate(builder.CreateCall3(Intrinsic::getDeclaration(builder.resources->module, Intrinsic::fma, args), ac, bc, cc), ac);
    }
};
LIKELY_REGISTER(fma)

class selectExpression : public SimpleTernaryOperator
{
    Expression *evaluateSimpleTernary(Builder &builder, const UniqueExpression &c, const UniqueExpression &t, const UniqueExpression &f) const
    {
        const likely_type type = likely_type_from_types(t, f);
        return new Immediate(builder.CreateSelect(c, builder.cast(&t, type), builder.cast(&f, type)), type);
    }
};
LIKELY_REGISTER(select)

struct Definition : public ScopedExpression
{
    Definition(Builder &builder, likely_const_ast ast)
        : ScopedExpression(builder, ast) {}

private:
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_env restored = builder.env;
        builder.env = this->env;
        unique_ptr<Expression> op(builder.expression(this->ast));
        builder.env = restored;
        return op->evaluate(builder, ast);
    }
};

class defineExpression : public Operator
{
    size_t maxParameters() const { return 2; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        if (ast->atoms[1]->is_list)
            return error(ast->atoms[1], "expected an atom");
        builder.define(ast->atoms[1]->atom, new Definition(builder, ast->atoms[2]));
        return NULL;
    }
};
LIKELY_REGISTER_EXPRESSION(define, "=")

class Variable : public Operator
{
    likely_type type_;
    AllocaInst *allocaInst;

public:
    Variable(Builder &builder, Expression *expr, const string &name)
    {
        type_ = *expr;
        allocaInst = builder.CreateAlloca(expr->value()->getType(), 0, name);
        set(builder, expr);
    }

    void set(Builder &builder, Expression *expr)
    {
        builder.CreateStore(builder.cast(expr, type_), allocaInst);
    }

private:
    size_t maxParameters() const { return 0; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast) const
    {
        return new Immediate(builder.CreateLoad(allocaInst), type_);
    }
};

class setExpression : public Operator
{
    size_t maxParameters() const { return 2; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        if (ast->atoms[1]->is_list)
            return error(ast->atoms[1], "expected an atom");
        const string name = ast->atoms[1]->atom;
        Expression *expr = builder.expression(ast->atoms[2]);
        if (expr) {
            UniqueExpression &variable = builder.locals[name];
            if (variable.isNull()) variable = new Variable(builder, expr, name);
            else                   static_cast<Variable*>(variable.get())->set(builder, expr);
        }
        return expr;
    }
};
LIKELY_REGISTER(set)

class compositionExpression : public Operator
{
    size_t maxParameters() const { return 2; }

    static bool isInt(likely_const_ast atom)
    {
        if (atom->is_list)
            return false;
        char *p;
        strtol(atom->atom, &p, 10);
        return *p == 0;
    }

    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        UniqueAST composed;
        if (isInt(ast->atoms[1]) && isInt(ast->atoms[2])) {
            stringstream stream;
            stream << ast->atoms[1]->atom << "." << ast->atoms[2]->atom;
            composed = likely_new_atom(stream.str().data());
        } else {
            vector<likely_const_ast> atoms(2);
            atoms[0] = likely_retain_ast(ast->atoms[2]);
            atoms[1] = likely_retain_ast(ast->atoms[1]);
            composed = likely_new_list(atoms.data(), 2);
        }
        return builder.expression(composed.get());
    }
};
LIKELY_REGISTER_EXPRESSION(composition, ".")

class elementsExpression : public SimpleUnaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(NativeInt, T::Void, false);
        Function *likelyElements = builder.resources->module->getFunction("likely_elements");
        if (!likelyElements) {
            likelyElements = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_elements", builder.resources->module);
            likelyElements->setCallingConv(CallingConv::C);
            likelyElements->setDoesNotAlias(1);
            likelyElements->setDoesNotCapture(1);
        }
        return new Immediate(builder.CreateCall(likelyElements, arg), likely_type_native);
    }
    void *symbol() const { return (void*) likely_elements; }
};
LIKELY_REGISTER(elements)

class bytesExpression : public SimpleUnaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(NativeInt, T::Void, false);
        Function *likelyBytes = builder.resources->module->getFunction("likely_bytes");
        if (!likelyBytes) {
            likelyBytes = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_bytes", builder.resources->module);
            likelyBytes->setCallingConv(CallingConv::C);
            likelyBytes->setDoesNotAlias(1);
            likelyBytes->setDoesNotCapture(1);
        }
        return new Immediate(builder.CreateCall(likelyBytes, arg), likely_type_native);
    }
    void *symbol() const { return (void*) likely_bytes; }
};
LIKELY_REGISTER(bytes)

class newExpression : public Operator, public LibraryFunction
{
    size_t maxParameters() const { return 6; }
    size_t minParameters() const { return 0; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const size_t n = ast->num_atoms - 1;
        UniqueExpression type;
        Value *channels, *columns, *rows, *frames, *data;
        switch (n) {
            case 6: data     = builder.expression(ast->atoms[6])->take();
            case 5: frames   = builder.expression(ast->atoms[5])->take();
            case 4: rows     = builder.expression(ast->atoms[4])->take();
            case 3: columns  = builder.expression(ast->atoms[3])->take();
            case 2: channels = builder.expression(ast->atoms[2])->take();
            case 1: type     = builder.expression(ast->atoms[1]);
            default:           break;
        }

        switch (maxParameters()-n) {
            case 6: type     = new Immediate(Builder::type(Builder::validFloatType(likely_type_native)));
            case 5: channels = Builder::one();
            case 4: columns  = Builder::one();
            case 3: rows     = Builder::one();
            case 2: frames   = Builder::one();
            case 1: data     = Builder::nullData();
            default:           break;
        }

        return new Immediate(createCall(builder, *type, channels, columns, rows, frames, data), likely_type_void);
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

        Function *likelyNew = builder.resources->module->getFunction("likely_new");
        if (!likelyNew) {
            likelyNew = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_new", builder.resources->module);
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
    Expression *evaluateUnary(Builder &builder, likely_const_ast arg) const
    {
        Expression *argExpr = builder.expression(arg);
        if (!argExpr)
            return NULL;

        if (argExpr->value()->getType() == T::Void)
            return argExpr;

        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> params;
            params.push_back(Type::getDoubleTy(C));
            params.push_back(NativeInt);
            functionType = FunctionType::get(T::Void, params, false);
        }
        Function *likelyScalar = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_scalar", builder.resources->module);
        likelyScalar->setCallingConv(CallingConv::C);
        likelyScalar->setDoesNotAlias(0);

        Immediate result(builder.CreateCall2(likelyScalar, builder.cast(argExpr, likely_type_f64), Builder::type(argExpr->type())), argExpr->type());
        delete argExpr;
        return new Immediate(result);
    }

    void *symbol() const { return (void*) likely_scalar; }
};
LIKELY_REGISTER(scalar)

class stringExpression : public SimpleUnaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new Immediate(createCall(builder, arg), likely_type_i8);
    }

    void *symbol() const { return (void*) likely_string; }

public:
    static CallInst *createCall(Builder &builder, Value *string)
    {
        static FunctionType *functionType = FunctionType::get(T::Void, Type::getInt8PtrTy(C), false);
        Function *likelyString = builder.resources->module->getFunction("likely_string");
        if (!likelyString) {
            likelyString = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_string", builder.resources->module);
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
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new Immediate(createCall(builder, arg), arg);
    }

    void *symbol() const { return (void*) likely_copy; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(T::Void, T::Void, false);
        Function *likelyCopy = builder.resources->module->getFunction("likely_copy");
        if (!likelyCopy) {
            likelyCopy = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_copy", builder.resources->module);
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
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new Immediate(createCall(builder, arg), arg);
    }

    void *symbol() const { return (void*) likely_retain; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(T::Void, T::Void, false);
        Function *likelyRetain = builder.resources->module->getFunction("likely_retain");
        if (!likelyRetain) {
            likelyRetain = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_retain", builder.resources->module);
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
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        return new Immediate(createCall(builder, arg), arg);
    }

    void *symbol() const { return (void*) likely_release; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(Type::getVoidTy(C), T::Void, false);
        Function *likelyRelease = builder.resources->module->getFunction("likely_release");
        if (!likelyRelease) {
            likelyRelease = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_release", builder.resources->module);
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

    Immediate generate(Builder &builder, vector<T> types, string name) const
    {
        size_t n;
        if (ast->is_list && (ast->num_atoms > 1))
            if (ast->atoms[1]->is_list) n = ast->atoms[1]->num_atoms;
            else                        n = 1;
        else                            n = 0;

        while (types.size() < n)
            types.push_back(T::get(likely_type_void));

        Function *tmpFunction = cast<Function>(builder.resources->module->getOrInsertFunction(name+"_tmp", FunctionType::get(Type::getVoidTy(C), T::toLLVM(types), false)));
        vector<Immediate> tmpArgs = builder.getArgs(tmpFunction, types);
        BasicBlock *entry = BasicBlock::Create(C, "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        Expression *result = evaluateFunction(builder, tmpArgs);

        if (!result)
            return Immediate(NULL, likely_type_void);
        builder.CreateRet(*result);
        likely_type return_type = result->type();

        Function *function = cast<Function>(builder.resources->module->getOrInsertFunction(name, FunctionType::get(result->value()->getType(), T::toLLVM(types), false)));
        vector<Immediate> args = builder.getArgs(function, types);

        ValueToValueMapTy VMap;
        for (size_t i=0; i<args.size(); i++)
            VMap[tmpArgs[i]] = args[i];

        SmallVector<ReturnInst*, 1> returns;
        CloneFunctionInto(function, tmpFunction, VMap, false, returns);
        tmpFunction->eraseFromParent();
        delete result;
        return Immediate(function, return_type);
    }

private:
    void *symbol() const { return (void*) likely_dynamic; }

    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const size_t parameters = this->ast->atoms[1]->is_list ? this->ast->atoms[1]->num_atoms : 1;
        const size_t arguments = ast->is_list ? ast->num_atoms - 1 : 0;
        if (parameters != arguments)
            return errorArgc(ast, "lambda", arguments, parameters, parameters);

        vector<Immediate> args;
        for (size_t i=0; i<arguments; i++) {
            TRY_EXPR(builder, ast->atoms[i+1], arg)
            args.push_back(Immediate(arg.value(), arg.type()));
        }

        likely_env this_env = env;
        swap(builder.env, this_env);
        Expression *result = evaluateFunction(builder, args);
        swap(builder.env, this_env);
        return result;
    }

    Expression *evaluateFunction(Builder &builder, const vector<Immediate> &args) const
    {
        // Do dynamic dispatch if the type isn't fully specified
        bool dynamic = false;
        for (const Immediate &arg : args)
            dynamic = dynamic || (arg.type_ == likely_type_void);
        dynamic = dynamic || (args.size() < ast->atoms[1]->num_atoms);

        if (dynamic) {
            VTable *vTable = new VTable(builder, ast, args.size());
            builder.resources->expressions.push_back(vTable);

            static PointerType *vTableType = PointerType::getUnqual(StructType::create(C, "VTable"));
            static FunctionType *likelyDynamicType = NULL;
            if (likelyDynamicType == NULL) {
                vector<Type*> params;
                params.push_back(vTableType);
                params.push_back(T::Void);
                likelyDynamicType = FunctionType::get(T::Void, params, true);
            }

            Function *likelyDynamic = builder.resources->module->getFunction("likely_dynamic");
            if (!likelyDynamic) {
                likelyDynamic = Function::Create(likelyDynamicType, GlobalValue::ExternalLinkage, "likely_dynamic", builder.resources->module);
                likelyDynamic->setCallingConv(CallingConv::C);
                likelyDynamic->setDoesNotAlias(0);
                likelyDynamic->setDoesNotAlias(1);
                likelyDynamic->setDoesNotCapture(1);
                likelyDynamic->setDoesNotAlias(2);
                likelyDynamic->setDoesNotCapture(2);
            }

            vector<Value*> dynamicArgs;
            dynamicArgs.push_back(ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(C, 8*sizeof(vTable)), uintptr_t(vTable)), vTableType));
            for (const Immediate &arg : args)
                dynamicArgs.push_back(arg);
            dynamicArgs.push_back(Builder::nullMat());
            return new Immediate(builder.CreateCall(likelyDynamic, dynamicArgs), likely_type_void);
        }

        return evaluateLambda(builder, args);
    }

    virtual Expression *evaluateLambda(Builder &builder, const vector<Immediate> &args) const
    {
        if (ast->atoms[1]->is_list) {
            for (size_t i=0; i<args.size(); i++)
                builder.define(ast->atoms[1]->atoms[i]->atom, args[i]);
        } else {
            builder.define(ast->atoms[1]->atom, args[0]);
        }

        Expression *result = builder.expression(ast->atoms[2]);

        if (ast->atoms[1]->is_list) {
            for (size_t i=0; i<args.size(); i++)
                builder.undefine(ast->atoms[1]->atoms[i]->atom);
        } else {
            builder.undefine(ast->atoms[1]->atom);
        }

        return result;
    }
};

class lambdaExpression : public Operator
{
    size_t maxParameters() const { return 2; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
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

    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
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

    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        for (size_t i=1; i<ast->num_atoms-1; i++) {
            TRY_EXPR(builder, ast->atoms[i], expr)
        }
        return builder.expression(ast->atoms[ast->num_atoms-1]);
    }
};
LIKELY_REGISTER_EXPRESSION(begin, "{")

struct Label : public Expression
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

    Expression *evaluate(Builder &builder, likely_const_ast) const
    {
        builder.CreateBr(basicBlock);
        return new Label(basicBlock);
    }
};

class labelExpression : public Operator
{
    size_t maxParameters() const { return 1; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const string name = ast->atoms[1]->atom;
        BasicBlock *label = BasicBlock::Create(C, name, builder.GetInsertBlock()->getParent());
        builder.CreateBr(label);
        builder.SetInsertPoint(label);
        builder.locals.insert(pair<string,UniqueExpression>(name, new Label(label)));
        return new Label(label);
    }
};
LIKELY_REGISTER(label)

class ifExpression : public Operator
{
    size_t minParameters() const { return 2; }
    size_t maxParameters() const { return 3; }

    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
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
        if (!True->back().isTerminator())
            builder.CreateBr(End);

        if (hasElse) {
            builder.SetInsertPoint(False);
            TRY_EXPR(builder, ast->atoms[3], f)
            if (!False->back().isTerminator())
                builder.CreateBr(End);

            builder.SetInsertPoint(End);
            PHINode *phi = builder.CreatePHI(t.value()->getType(), 2);
            phi->addIncoming(t, True);
            if (hasElse) phi->addIncoming(f, False);
            return new Immediate(phi, t);
        } else {
            builder.SetInsertPoint(End);
            return new Immediate(NULL, likely_type_void);
        }
    }
};
LIKELY_REGISTER(if)

struct Kernel : public Lambda
{
    Kernel(Builder &builder, likely_const_ast ast)
        : Lambda(builder, ast) {}

private:
    class kernelArgument : public Operator
    {
        Immediate matrix;
        likely_type kernel;
        MDNode *node;

    public:
        kernelArgument(const Immediate &matrix, likely_type kernel, MDNode *node)
            : matrix(matrix), kernel(kernel), node(node) {}

    private:
        size_t maxParameters() const { return 0; }
        Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
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
            return new Immediate(load, matrix);
        }
    };

    Expression *evaluateLambda(Builder &builder, const vector<Immediate> &srcs) const
    {
        vector<T> types;
        for (const Immediate &src : srcs)
            types.push_back(T(src.value_->getType(), src.type_));

        BasicBlock *entry = builder.GetInsertBlock();
        Immediate dstChannels = getDimensions(builder, ast, "channels", srcs);
        Immediate dstColumns  = getDimensions(builder, ast, "columns" , srcs);
        Immediate dstRows     = getDimensions(builder, ast, "rows"    , srcs);
        Immediate dstFrames   = getDimensions(builder, ast, "frames"  , srcs);

        Function *thunk;
        likely_type dstType;
        {
            thunk = getKernel(builder, getUniqueName("thunk"), types, Type::getVoidTy(C), T::Void, NativeInt, NativeInt);
            BasicBlock *entry = BasicBlock::Create(C, "entry", thunk);
            builder.SetInsertPoint(entry);
            vector<Immediate> srcs = builder.getArgs(thunk, types);
            Immediate stop = srcs.back(); srcs.pop_back();
            stop.type_ = likely_type_native;
            stop.value_->setName("stop");
            Immediate start = srcs.back(); srcs.pop_back();
            start.type_ = likely_type_native;
            start.value_->setName("start");
            Immediate dst = srcs.back(); srcs.pop_back();
            dst.value_->setName("dst");

            likely_set_multi_channel(&dst.type_, likely_multi_channel(dstChannels));
            likely_set_multi_column (&dst.type_, likely_multi_column (dstColumns));
            likely_set_multi_row    (&dst.type_, likely_multi_row    (dstRows));
            likely_set_multi_frame  (&dst.type_, likely_multi_frame  (dstFrames));

            // Create self-referencing loop node
            MDNode *node; {
                vector<Value*> metadata;
                MDNode *tmp = MDNode::getTemporary(C, metadata);
                metadata.push_back(tmp);
                node = MDNode::get(C, metadata);
                tmp->replaceAllUsesWith(node);
                MDNode::deleteTemporary(tmp);
            }

            // The kernel assumes there is at least one iteration
            BasicBlock *body = BasicBlock::Create(C, "kernel_body", thunk);
            builder.CreateBr(body);
            builder.SetInsertPoint(body);
            PHINode *i = builder.CreatePHI(NativeInt, 2, "i");
            i->addIncoming(start, entry);

            Value *columnStep, *rowStep, *frameStep;
            builder.steps(&dst, &columnStep, &rowStep, &frameStep);
            Value *frameRemainder = builder.CreateURem(i, frameStep, "t_rem");
            Immediate t(builder.CreateUDiv(i, frameStep, "t"), likely_type_native);
            Value *rowRemainder = builder.CreateURem(frameRemainder, rowStep, "y_rem");
            Immediate y(builder.CreateUDiv(frameRemainder, rowStep, "y"), likely_type_native);
            Value *columnRemainder = builder.CreateURem(rowRemainder, columnStep, "c");
            Immediate x(builder.CreateUDiv(rowRemainder, columnStep, "x"), likely_type_native);
            Immediate c(columnRemainder, likely_type_native);

            builder.define("i", i, likely_type_native);
            builder.define("c", c, likely_type_native);
            builder.define("x", x, likely_type_native);
            builder.define("y", y, likely_type_native);
            builder.define("t", t, likely_type_native);

            const likely_const_ast args = ast->atoms[1];
            if (args->is_list) {
                assert(srcs.size() == args->num_atoms);
                for (size_t j=0; j<args->num_atoms; j++)
                    builder.define(args->atoms[j]->atom, new kernelArgument(srcs[j], dst, node));
            } else {
                assert(srcs.size() == 1);
                builder.define(args->atom, new kernelArgument(srcs[0], dst, node));
            }

            UniqueExpression result(builder.expression(ast->atoms[2]));
            dstType = dst.type_ = result;
            StoreInst *store = builder.CreateStore(result, builder.CreateGEP(builder.data(&dst), i));
            store->setMetadata("llvm.mem.parallel_loop_access", node);

            Value *increment = builder.CreateAdd(i, builder.one(), "kernel_increment");
            BasicBlock *loopLatch = BasicBlock::Create(C, "kernel_latch", thunk);
            builder.CreateBr(loopLatch);
            builder.SetInsertPoint(loopLatch);
            BasicBlock *loopExit = BasicBlock::Create(C, "kernel_exit", thunk);
            BranchInst *latch = builder.CreateCondBr(builder.CreateICmpEQ(increment, stop, "kernel_test"), loopExit, body);
            latch->setMetadata("llvm.loop", node);
            i->addIncoming(increment, loopLatch);
            builder.SetInsertPoint(loopExit);
            builder.CreateRetVoid();

            if (args->is_list) {
                for (size_t i=0; i<args->num_atoms; i++)
                    builder.undefine(args->atoms[i]->atom);
            } else {
                builder.undefine(args->atom);
            }

            builder.undefine("i");
            builder.undefine("c");
            builder.undefine("x");
            builder.undefine("y");
            builder.undefine("t");
        }

        builder.SetInsertPoint(entry);

        Value *dst = newExpression::createCall(builder, Builder::type(dstType), dstChannels, dstColumns, dstRows, dstFrames, Builder::nullData());

        Value *kernelSize = builder.CreateMul(builder.CreateMul(builder.CreateMul(dstChannels, dstColumns), dstRows), dstFrames);

        if (!srcs.empty() && likely_parallel(srcs[0])) {
            vector<Type*> likelyForkParameters;
            likelyForkParameters.push_back(thunk->getType());
            likelyForkParameters.push_back(Type::getInt8Ty(C));
            likelyForkParameters.push_back(NativeInt);
            likelyForkParameters.push_back(T::Void);
            Type *likelyForkReturn = Type::getVoidTy(C);
            FunctionType *likelyForkType = FunctionType::get(likelyForkReturn, likelyForkParameters, true);

            Function *likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "likely_fork", builder.resources->module);
            likelyFork->setCallingConv(CallingConv::C);
            likelyFork->setDoesNotCapture(4);
            likelyFork->setDoesNotAlias(4);

            vector<Value*> likelyForkArgs;
            likelyForkArgs.push_back(builder.resources->module->getFunction(thunk->getName()));
            likelyForkArgs.push_back(Builder::constant(uint64_t(srcs.size()), likely_type_u8));
            likelyForkArgs.push_back(kernelSize);
            for (const Immediate &src : srcs)
                likelyForkArgs.push_back(builder.CreatePointerCast(src, T::Void));
            likelyForkArgs.push_back(dst);
            builder.CreateCall(likelyFork, likelyForkArgs);
        } else {
            vector<Value*> thunkArgs;
            for (const Immediate &src : srcs)
                thunkArgs.push_back(src);
            thunkArgs.push_back(dst);
            thunkArgs.push_back(Builder::zero());
            thunkArgs.push_back(kernelSize);
            builder.CreateCall(thunk, thunkArgs);
        }

        return new Immediate(dst, dstType);
    }

    static Function *getKernel(Builder &builder, const string &name, const vector<T> &types, Type *result, Type *dst = NULL, Type *start = NULL, Type *stop = NULL)
    {
        vector<Type*> params = T::toLLVM(types);
        if (dst)   params.push_back(dst);
        if (start) params.push_back(start);
        if (stop)  params.push_back(stop);
        Function *kernel = ::cast<Function>(builder.resources->module->getOrInsertFunction(name, FunctionType::get(result, params, false)));
        kernel->addFnAttr(Attribute::NoUnwind);
        kernel->setCallingConv(CallingConv::C);
        if (result->isPointerTy())
            kernel->setDoesNotAlias(0);
        size_t argc = types.size();
        if (dst) argc++;
        for (size_t i=0; i<argc; i++) {
            kernel->setDoesNotAlias((unsigned int) i+1);
            kernel->setDoesNotCapture((unsigned int) i+1);
        }
        return kernel;
    }

    static Immediate getDimensions(Builder &builder, likely_const_ast ast, const char *axis, const vector<Immediate> &srcs)
    {
        Value *result = NULL;
        // Look for a dimensionality expression
        if (ast->num_atoms == 4) {
            vector<pair<likely_const_ast,likely_const_ast>> pairs;
            getPairs(ast->atoms[3], pairs);
            for (const auto &pair : pairs)
                if (!strcmp(axis, pair.first->atom)) {
                    result = builder.cast(unique_ptr<Expression>(builder.expression(pair.second)).get(), likely_type_native);
                    break;
                }
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

        likely_type type = likely_type_native;
        const bool isMulti = (!LLVM_VALUE_IS_INT(result)) || (LLVM_VALUE_TO_INT(result) > 1);
        if      (!strcmp(axis, "channels")) likely_set_multi_channel(&type, isMulti);
        else if (!strcmp(axis, "columns"))  likely_set_multi_column (&type, isMulti);
        else if (!strcmp(axis, "rows"))     likely_set_multi_row    (&type, isMulti);
        else                                likely_set_multi_frame  (&type, isMulti);

        return Immediate(result, type);
    }
};

class kernelExpression : public Operator
{
    size_t minParameters() const { return 2; }
    size_t maxParameters() const { return 3; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        return new Kernel(builder, ast);
    }
};
LIKELY_REGISTER_EXPRESSION(kernel, "=>")

class exportExpression : public Operator
{
    size_t maxParameters() const { return 3; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
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

JITResources::JITResources(likely_const_ast ast, likely_env env, const vector<likely_type> &type)
    : Resources(true), type(type)
{
#ifdef _WIN32
    module->setTargetTriple(module->getTargetTriple() + "-elf");
#endif

    string error;
    EngineBuilder engineBuilder(module);
    engineBuilder.setMCPU(sys::getHostCPUName())
                 .setOptLevel(CodeGenOpt::Aggressive)
                 .setErrorStr(&error)
                 .setEngineKind(EngineKind::JIT)
                 .setUseMCJIT(true);
    EE = engineBuilder.create();
    likely_assert(EE != NULL, "failed to create execution engine with error: %s", error.c_str());

    likely_assert(ast->is_list && (ast->num_atoms > 0) && !ast->atoms[0]->is_list &&
                  (!strcmp(ast->atoms[0]->atom, "->") || !strcmp(ast->atoms[0]->atom, "=>")),
                  "expected a lambda expression");
    Builder builder(this, env);
    unique_ptr<Expression> result(builder.expression(ast));

    vector<T> types;
    for (likely_type t : type)
        types.push_back(T::get(t));
    Function *F = dyn_cast_or_null<Function>(static_cast<Lambda*>(result.get())->generate(builder, types, getUniqueName("jit")).value_);

    if (F) {
        optimize();
        EE->finalizeObject();
        function = EE->getPointerToFunction(F);
    }
}

struct OfflineEnvironment : public likely_environment
{
    unique_ptr<OfflineResources> resources;

    OfflineEnvironment(const string &fileName, bool native)
        : resources(new OfflineResources(fileName, native))
    {}

private:
    likely_mat evaluate(likely_const_ast ast)
    {
        Builder builder(resources.get(), this);
        delete builder.expression(ast);
        return NULL;
    }
};

#ifdef LIKELY_IO
#include "likely/io.h"

class printExpression : public Operator, public LibraryFunction
{
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }
    size_t minParameters() const { return 0; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        static FunctionType *functionType = FunctionType::get(T::Void, T::Void, true);
        Function *likelyPrint = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_print", builder.resources->module);
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

        return new Immediate(result, likely_type_i8);
    }

    void *symbol() const { return (void*) likely_print; }
};
LIKELY_REGISTER(print)

class readExpression : public SimpleUnaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(T::Void, Type::getInt8PtrTy(C), false);
        Function *likelyRead = builder.resources->module->getFunction("likely_read");
        if (!likelyRead) {
            likelyRead = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_read", builder.resources->module);
            likelyRead->setCallingConv(CallingConv::C);
            likelyRead->setDoesNotAlias(0);
            likelyRead->setDoesNotAlias(1);
            likelyRead->setDoesNotCapture(1);
        }
        return new Immediate(builder.CreateCall(likelyRead, arg), likely_type_void);
    }
    void *symbol() const { return (void*) likely_read; }
};
LIKELY_REGISTER(read)

class writeExpression : public SimpleBinaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &arg1, const UniqueExpression &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> likelyWriteParameters;
            likelyWriteParameters.push_back(T::Void);
            likelyWriteParameters.push_back(Type::getInt8PtrTy(C));
            functionType = FunctionType::get(T::Void, likelyWriteParameters, false);
        }
        Function *likelyWrite = builder.resources->module->getFunction("likely_write");
        if (!likelyWrite) {
            likelyWrite = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_write", builder.resources->module);
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
        return new Immediate(builder.CreateCall(likelyWrite, likelyWriteArguments), likely_type_void);
    }
    void *symbol() const { return (void*) likely_write; }
};
LIKELY_REGISTER(write)

class decodeExpression : public SimpleUnaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleUnary(Builder &builder, const UniqueExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(T::Void, T::Void, false);
        Function *likelyDecode = builder.resources->module->getFunction("likely_decode");
        if (!likelyDecode) {
            likelyDecode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_decode", builder.resources->module);
            likelyDecode->setCallingConv(CallingConv::C);
            likelyDecode->setDoesNotAlias(0);
            likelyDecode->setDoesNotAlias(1);
            likelyDecode->setDoesNotCapture(1);
        }
        return new Immediate(builder.CreateCall(likelyDecode, arg), likely_type_void);
    }
    void *symbol() const { return (void*) likely_decode; }
};
LIKELY_REGISTER(decode)

class encodeExpression : public SimpleBinaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleBinary(Builder &builder, const UniqueExpression &arg1, const UniqueExpression &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> parameters;
            parameters.push_back(T::Void);
            parameters.push_back(Type::getInt8PtrTy(C));
            functionType = FunctionType::get(T::Void, parameters, false);
        }
        Function *likelyEncode = builder.resources->module->getFunction("likely_encode");
        if (!likelyEncode) {
            likelyEncode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_encode", builder.resources->module);
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
        return new Immediate(builder.CreateCall(likelyEncode, likelyEncodeArguments), likely_type_void);
    }
    void *symbol() const { return (void*) likely_encode; }
};
LIKELY_REGISTER(encode)
#endif // LIKELY_IO

} // namespace (anonymous)

likely_env likely_new_jit()
{
    return new likely_environment();
}

likely_env likely_new_offline(const char *file_name, bool native)
{
    return new OfflineEnvironment(file_name, native);
}

likely_env likely_retain_env(likely_const_env env)
{
    if (env) env->ref_count++;
    return (likely_env) env;
}

void likely_release_env(likely_const_env env)
{
    if (!env || --env->ref_count) return;
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
        const JITResources *resources = vTable->functions[i];
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
        JITResources *resources = new JITResources(vTable->ast, vTable->env, types);
        vTable->functions.push_back(resources);
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

static map<likely_function, pair<JITResources*,int>> ResourcesLUT;

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
    JITResources *r = new JITResources(ast, env, types);
    likely_function f = reinterpret_cast<likely_function>(r->function);
    if (f) ResourcesLUT[f] = pair<JITResources*,int>(r, 1);
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
    pair<JITResources*,int> &df = ResourcesLUT[function];
    if (--df.second) return;
    ResourcesLUT.erase(function);
    delete df.first;
}

likely_mat likely_eval(likely_const_ast ast, likely_env env)
{
    if (!ast || !env) return NULL;
    return env->evaluate(ast);
}
