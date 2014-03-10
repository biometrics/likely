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
#include <iostream>
#include <memory>
#include <sstream>

#include "likely/likely_backend.h"

#define LLVM_VALUE_IS_INT(VALUE) (llvm::isa<Constant>(VALUE))
#define LLVM_VALUE_TO_INT(VALUE) (llvm::cast<Constant>(VALUE)->getUniqueInteger().getZExtValue())

using namespace llvm;
using namespace std;

namespace {

static IntegerType *NativeIntegerType = NULL;
static PointerType *Mat = NULL;
static LLVMContext &C = getGlobalContext();

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

class ManagedExpression : public Expression
{
    Expression *e;

public:
    ManagedExpression(Expression *e = NULL) : e(e) {}
    ~ManagedExpression() { delete e; }
    bool isNull() const { return !e; }
    Value* value() const { return e->value(); }
    likely_type type() const { return e->type(); }
    Expression *evaluate(Builder &builder, likely_const_ast ast) const { return e->evaluate(builder, ast); }
};

#define TRY_EXPR(BUILDER, AST, EXPR)                     \
const ManagedExpression EXPR((BUILDER).expression(AST)); \
if (EXPR.isNull()) return NULL;                          \

} // namespace (anonymous)

struct likely_environment : public map<string,stack<shared_ptr<Expression>>>
{
    static likely_environment defaultExprs;
    mutable int ref_count = 1;
    likely_environment(const map<string,stack<shared_ptr<Expression>>> &exprs = defaultExprs)
        : map<string,stack<shared_ptr<Expression>>>(exprs) {}
};
likely_environment likely_environment::defaultExprs;

namespace {

static string getUniqueName(const string &prefix)
{
    static map<string, int> uidLUT;
    stringstream stream;
    stream << "likely_" << prefix << "_" << uidLUT[prefix]++;
    return stream.str();
}

struct Immediate : public Expression
{
    Value *value_;
    likely_type type_;

    Immediate(Value *value, likely_type type)
        : value_(value), type_(type) {}

private:
    Value *value() const { return value_; }
    likely_type type() const { return type_; }
    Expression *evaluate(Builder &, likely_const_ast) const
    {
        return new Immediate(value(), type());
    }
};

struct Object
{
    virtual ~Object() {}
};

struct Resources : public Object
{
    Module *module;
    ExecutionEngine *executionEngine = NULL;
    TargetMachine *targetMachine = NULL;
    void *function = NULL;
    vector<Object*> children;

    Resources(bool native, bool JIT)
    {
        if (!Mat) {
            assert(sizeof(likely_size) == sizeof(void*));
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

            NativeIntegerType = Type::getIntNTy(C, likely_depth(likely_type_native));
            Mat = PointerType::getUnqual(StructType::create("likely_matrix",
                                                            NativeIntegerType, // bytes
                                                            NativeIntegerType, // ref_count
                                                            NativeIntegerType, // channels
                                                            NativeIntegerType, // columns
                                                            NativeIntegerType, // rows
                                                            NativeIntegerType, // frames
                                                            NativeIntegerType, // type
                                                            ArrayType::get(Type::getInt8Ty(C), 0), // data
                                                            NULL));
        }

        module = new Module(getUniqueName("module"), C);
        likely_assert(module != NULL, "failed to create module");

        string error;
        EngineBuilder engineBuilder(module);
        engineBuilder.setMCPU(sys::getHostCPUName())
                     .setOptLevel(CodeGenOpt::Aggressive)
                     .setErrorStr(&error);

        if (native) {
            static string nativeTT, nativeJITTT;
            if (nativeTT.empty()) {
                nativeTT = sys::getProcessTriple();
#ifdef _WIN32
                nativeJITTT = nativeTT + "-elf";
#else
                nativeJITTT = nativeTT;
#endif // _WIN32
            }
            module->setTargetTriple(JIT ? nativeJITTT : nativeTT);

            static TargetMachine *nativeTM = NULL;
            if (!nativeTM) {
                engineBuilder.setCodeModel(CodeModel::Default);
                nativeTM = engineBuilder.selectTarget();
                likely_assert(nativeTM != NULL, "failed to select target machine with error: %s", error.c_str());
            }
            targetMachine = nativeTM;
        }

        if (JIT) {
            engineBuilder.setCodeModel(CodeModel::JITDefault)
                         .setEngineKind(EngineKind::JIT)
                         .setUseMCJIT(true);
            executionEngine = engineBuilder.create();
            likely_assert(executionEngine != NULL, "failed to create execution engine with error: %s", error.c_str());
        }
    }

    ~Resources()
    {
        for (Object *child : children)
            delete child;
        if (executionEngine) delete executionEngine; // owns module
        else                 delete module;
    }

    void *finalize(Function *F)
    {
        if (!F)
            return NULL;

        if (targetMachine) {
            static PassManager *PM = NULL;
            if (!PM) {
                PM = new PassManager();
                PM->add(createVerifierPass());
                PM->add(new TargetLibraryInfo(Triple(module->getTargetTriple())));
                PM->add(new DataLayoutPass(*targetMachine->getDataLayout()));
                targetMachine->addAnalysisPasses(*PM);
                PassManagerBuilder builder;
                builder.OptLevel = 3;
                builder.SizeLevel = 0;
                builder.LoopVectorize = true;
                builder.populateModulePassManager(*PM);
                PM->add(createVerifierPass());
            }

//            DebugFlag = true;
//            module->dump();
            PM->run(*module);
//            module->dump();
        }

        if (executionEngine) {
            executionEngine->finalizeObject();
            function = executionEngine->getPointerToFunction(F);
        }

        return function;
    }
};

struct Builder : public IRBuilder<>
{
    likely_env env;
    Resources *resources;

    Builder(Resources *resources, likely_env env)
        : IRBuilder<>(C), env(env), resources(resources)
    {}

    static Immediate constant(double value, likely_type type = likely_type_native)
    {
        const int depth = likely_depth(type);
        if (likely_floating(type)) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return Immediate(ConstantFP::get(Type::getDoubleTy(C), value), type);
            else if (depth == 32) return Immediate(ConstantFP::get(Type::getFloatTy(C), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return Immediate(NULL, likely_type_null); }
        } else {
            return Immediate(Constant::getIntegerValue(Type::getIntNTy(C, depth), APInt(depth, uint64_t(value))), type);
        }
    }

    static Immediate zero(likely_type type = likely_type_native) { return constant(0, type); }
    static Immediate one (likely_type type = likely_type_native) { return constant(1, type); }
    static Immediate intMax(likely_type type) { const int bits = likely_depth(type); return constant((1 << (bits - (likely_signed(type) ? 1 : 0)))-1, bits); }
    static Immediate intMin(likely_type type) { const int bits = likely_depth(type); return constant(likely_signed(type) ? (1 << (bits - 1)) : 0, bits); }
    static Immediate type(likely_type type) { return constant(type, likely_depth(likely_type_type)); }
    static Immediate nullMat() { return Immediate(ConstantPointerNull::get(Mat), likely_type_null); }
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

    vector<Immediate> getArgs(Function *function, const vector<likely_type> &types)
    {
        vector<Immediate> result;
        Function::arg_iterator args = function->arg_begin();
        likely_arity n = 0;
        while (args != function->arg_end()) {
            Value *src = args++;
            stringstream name; name << "arg_" << int(n);
            src->setName(name.str());
            result.push_back(Immediate(src, n < types.size() ? types[n] : likely_type_null));
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

    Expression *expression(likely_const_ast ast)
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
        if (type != likely_type_null)
            return new Immediate(constant(type, likely_type_u32));

        return Expression::error(ast, "unrecognized literal");
    }
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
        return likely_type_null;
    }

    Expression *evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (!ast->is_list && (minParameters() > 0))
            return error(ast, "operator expected arguments");
        const size_t args = ast->num_atoms - 1;
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

struct FunctionExpression : public ScopedExpression
{
    FunctionExpression(Builder &builder, likely_const_ast ast)
        : ScopedExpression(builder, ast) {}

    virtual Immediate generate(Builder &builder, const vector<likely_type> &types, string name = string()) const = 0;

private:
    size_t argc() const
    {
        // Where 'ast' is of the form:
        //     (lambda/kernel (arg0 arg1 ... argN) (expression))
        if (!ast->is_list || (ast->num_atoms < 2))
            return 0;
        if (!ast->atoms[1]->is_list)
            return 1;
        return ast->atoms[1]->num_atoms;
    }

    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        assert(ast->is_list);
        const size_t parameters = argc();
        const size_t arguments = ast->num_atoms - 1;
        if (parameters != arguments)
            return errorArgc(ast, "lambda", arguments, parameters, parameters);

        vector<Value*> args;
        vector<likely_type> types;
        for (size_t i=0; i<arguments; i++) {
            TRY_EXPR(builder, ast->atoms[i+1], arg)
            args.push_back(arg);
            types.push_back(arg);
        }

        Builder lambdaBuilder(builder.resources, env);
        Immediate i = generate(lambdaBuilder, types);
        Function *f = cast<Function>(i.value_);
        if (f) return new Immediate(builder.CreateCall(f, args), i.type_);
        else   return NULL;
    }
};

struct StaticFunction : public Resources
{
    const vector<likely_type> type;

    StaticFunction(likely_const_ast ast, likely_env env, const vector<likely_type> &type, bool native, string name = string())
        : Resources(native, name.empty()), type(type)
    {
        likely_assert(ast->is_list && (ast->num_atoms > 0) && !ast->atoms[0]->is_list &&
                      (!strcmp(ast->atoms[0]->atom, "lambda") || !strcmp(ast->atoms[0]->atom, "kernel")),
                      "expected a lambda/kernel expression");
        Builder builder(this, env);
        unique_ptr<Expression> result(builder.expression(ast));
        function = finalize(dyn_cast_or_null<Function>(static_cast<FunctionExpression*>(result.get())->generate(builder, type, name).value_));
    }

    void write(const string &fileName) const
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
            targetMachine->addPassesToEmitFile(pm, fos, extension == "s" ? TargetMachine::CGFT_AssemblyFile : TargetMachine::CGFT_ObjectFile);
            pm.run(*module);
        }

        likely_assert(errorInfo.empty(), "failed to write to: %s with error: %s", fileName.c_str(), errorInfo.c_str());
        output.keep();
    }
};

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

} // namespace (anonymous)

struct VTable : public ScopedExpression, public Object
{
    likely_arity n;
    vector<StaticFunction*> functions;

    VTable(Builder &builder, likely_const_ast ast)
        : ScopedExpression(builder, ast)
    {
        if (ast->is_list && (ast->num_atoms > 1))
            if (ast->atoms[1]->is_list) n = (likely_arity) ast->atoms[1]->num_atoms;
            else                        n = 1;
        else                            n = 0;
    }

    ~VTable()
    {
        for (StaticFunction *function : functions)
            delete function;
    }

    Expression *evaluateOperator(Builder &, likely_const_ast) const
    {
        return NULL;
    }
};

extern "C" LIKELY_EXPORT likely_const_mat likely_dynamic(struct VTable *vtable, likely_const_mat *m);

namespace {

struct DynamicFunction : public ScopedExpression, public LibraryFunction
{
    DynamicFunction(Builder &builder, likely_const_ast ast)
        : ScopedExpression(builder, ast)
    {}

    void *symbol() const { return (void*) likely_dynamic; }

    Expression *evaluateOperator(Builder &, likely_const_ast) const
    {
        return NULL;
    }

    Function *generate(Builder &builder)
    {
        VTable *vTable = new VTable(builder, ast);
        builder.resources->children.push_back(vTable);

        static FunctionType* functionType = FunctionType::get(Mat, Mat, true);

        Function *function = cast<Function>(builder.resources->module->getOrInsertFunction(getUniqueName("dynamic"), functionType));
        function->addFnAttr(Attribute::NoUnwind);
        function->setCallingConv(CallingConv::C);
        function->setDoesNotAlias(0);
        function->setDoesNotAlias(1);
        function->setDoesNotCapture(1);
        builder.SetInsertPoint(BasicBlock::Create(C, "entry", function));

        Value *array;
        if (vTable->n > 0) {
            array = builder.CreateAlloca(Mat, Constant::getIntegerValue(Type::getInt32Ty(C), APInt(32, (uint64_t)vTable->n)));
            builder.CreateStore(function->arg_begin(), builder.CreateGEP(array, Constant::getIntegerValue(NativeIntegerType, APInt(8*sizeof(void*), 0))));
            if (vTable->n > 1) {
                Value *vaList = builder.CreateAlloca(IntegerType::getInt8PtrTy(C));
                Value *vaListRef = builder.CreateBitCast(vaList, Type::getInt8PtrTy(C));
                builder.CreateCall(Intrinsic::getDeclaration(builder.resources->module, Intrinsic::vastart), vaListRef);
                for (likely_arity i=1; i<vTable->n; i++)
                    builder.CreateStore(builder.CreateVAArg(vaList, Mat), builder.CreateGEP(array, Constant::getIntegerValue(NativeIntegerType, APInt(8*sizeof(void*), i))));
                builder.CreateCall(Intrinsic::getDeclaration(builder.resources->module, Intrinsic::vaend), vaListRef);
            }
        } else {
            array = ConstantPointerNull::get(PointerType::getUnqual(Mat));
        }

        static PointerType *vTableType = PointerType::getUnqual(StructType::create(C, "VTable"));
        static FunctionType *likelyDynamicType = NULL;
        if (likelyDynamicType == NULL) {
            vector<Type*> params;
            params.push_back(vTableType);
            params.push_back(PointerType::getUnqual(Mat));
            likelyDynamicType = FunctionType::get(Mat, params, false);
        }

        Function *likelyDynamic = builder.resources->module->getFunction("likely_dynamic");
        if (!likelyDynamic) {
            likelyDynamic = Function::Create(likelyDynamicType, GlobalValue::ExternalLinkage, "likely_dynamic", builder.resources->module);
            likelyDynamic->setCallingConv(CallingConv::C);
            likelyDynamic->setDoesNotAlias(0);
            likelyDynamic->setDoesNotAlias(1);
            likelyDynamic->setDoesNotAlias(2);
            likelyDynamic->setDoesNotCapture(1);
            likelyDynamic->setDoesNotCapture(2);
        }

        Constant *thisVTableFunction = ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(C, 8*sizeof(vTable)), uintptr_t(vTable)), vTableType);
        builder.CreateRet(builder.CreateCall2(likelyDynamic, thisVTableFunction, array));
        return function;
    }
};

class dynamicExpression : public Operator
{
    size_t maxParameters() const { return 1; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        return new DynamicFunction(builder, ast->atoms[1]);
    }
};
LIKELY_REGISTER(dynamic)

#define LIKELY_REGISTER_TYPE(TYPE)                                             \
struct TYPE##Expression : public Immediate                                     \
{                                                                              \
    TYPE##Expression()                                                         \
        : Immediate(Builder::constant(likely_type_##TYPE, likely_type_u32)) {} \
};                                                                             \
LIKELY_REGISTER(TYPE)                                                          \

LIKELY_REGISTER_TYPE(null)
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
    virtual Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &arg) const = 0;
};

#define LIKELY_REGISTER_FIELD(FIELD)                                                      \
class FIELD##Expression : public SimpleUnaryOperator                                      \
{                                                                                         \
    Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &arg) const \
    {                                                                                     \
        return new Immediate(builder.FIELD(&arg));                                        \
    }                                                                                     \
};                                                                                        \
LIKELY_REGISTER(FIELD)                                                                    \

LIKELY_REGISTER_FIELD(channels)
LIKELY_REGISTER_FIELD(columns)
LIKELY_REGISTER_FIELD(rows)
LIKELY_REGISTER_FIELD(frames)

class notExpression : public SimpleUnaryOperator
{
    Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &arg) const
    {
        return new Immediate(builder.CreateXor(Builder::intMax(arg), arg.value()), arg);
    }
};
LIKELY_REGISTER_EXPRESSION(not, "~")

class typeExpression : public SimpleUnaryOperator
{
    Expression *evaluateSimpleUnary(Builder &, const ManagedExpression &arg) const
    {
        return new Immediate(Builder::type(arg));
    }
};
LIKELY_REGISTER(type)

class UnaryMathOperator : public SimpleUnaryOperator
{
    Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &x) const
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
    virtual Expression *evaluateSimpleBinary(Builder &builder, const ManagedExpression &arg1, const ManagedExpression &arg2) const = 0;
};

class castExpression : public SimpleBinaryOperator
{
    Expression *evaluateSimpleBinary(Builder &builder, const ManagedExpression &x, const ManagedExpression &type) const
    {
        return new Immediate(builder.cast(&x, (likely_type)LLVM_VALUE_TO_INT(type.value())));
    }
};
LIKELY_REGISTER(cast)

class ArithmeticOperator : public SimpleBinaryOperator
{
    Expression *evaluateSimpleBinary(Builder &builder, const ManagedExpression &lhs, const ManagedExpression &rhs) const
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

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                         \
class OP##Expression : public ArithmeticOperator                                                                    \
{                                                                                                                   \
    Expression *evaluateArithmetic(Builder &builder, const Immediate &lhs, const Immediate &rhs) const              \
    {                                                                                                               \
        return new Immediate(likely_floating(lhs) ? builder.CreateFCmpO##OP(lhs, rhs)                               \
                                                  : (likely_signed(lhs) ? builder.CreateICmpS##OP(lhs, rhs)         \
                                                                        : builder.CreateICmpU##OP(lhs, rhs)), lhs); \
    }                                                                                                               \
};                                                                                                                  \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                 \

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
                                                  : builder.CreateICmp##OP(lhs, rhs), lhs);            \
    }                                                                                                  \
};                                                                                                     \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                    \

LIKELY_REGISTER_EQUALITY(EQ, "==")
LIKELY_REGISTER_EQUALITY(NE, "!=")

class BinaryMathOperator : public SimpleBinaryOperator
{
    Expression *evaluateSimpleBinary(Builder &builder, const ManagedExpression &x, const ManagedExpression &n) const
    {
        const likely_type type = nIsInteger() ? x.type() : likely_type_from_types(x, n);
        Immediate xc(builder.cast(&x, Builder::validFloatType(type)));
        Immediate nc(builder.cast(&n, nIsInteger() ? likely_type_i32 : xc.type_));
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
    virtual Expression *evaluateSimpleTernary(Builder &builder, const ManagedExpression &arg1, const ManagedExpression &arg2, const ManagedExpression &arg3) const = 0;
};

class fmaExpression : public SimpleTernaryOperator
{
    Expression *evaluateSimpleTernary(Builder &builder, const ManagedExpression &a, const ManagedExpression &b, const ManagedExpression &c) const
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
    Expression *evaluateSimpleTernary(Builder &builder, const ManagedExpression &c, const ManagedExpression &t, const ManagedExpression &f) const
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
            return error(ast->atoms[1], "define expected an atom name");
        builder.define(ast->atoms[1]->atom, new Definition(builder, ast->atoms[2]));
        return NULL;
    }
};
LIKELY_REGISTER(define)

class newExpression : public Operator, public LibraryFunction
{
    size_t maxParameters() const { return 6; }
    size_t minParameters() const { return 0; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const size_t n = ast->num_atoms - 1;
        ManagedExpression type;
        Value *channels, *columns, *rows, *frames, *data;
        switch (n) {
            case 6: data     = builder.expression(ast->atoms[6])->take();
            case 5: frames   = builder.expression(ast->atoms[5])->take();
            case 4: rows     = builder.expression(ast->atoms[4])->take();
            case 3: columns  = builder.expression(ast->atoms[3])->take();
            case 2: channels = builder.expression(ast->atoms[2])->take();
            case 1: type     = ManagedExpression(builder.expression(ast->atoms[1]));
            default:           break;
        }

        switch (maxParameters()-n) {
            case 6: type     = ManagedExpression(new Immediate(Builder::type(Builder::validFloatType(likely_type_native))));
            case 5: channels = Builder::one();
            case 4: columns  = Builder::one();
            case 3: rows     = Builder::one();
            case 2: frames   = Builder::one();
            case 1: data     = Builder::nullData();
            default:           break;
        }

        return new Immediate(createCall(builder, type, channels, columns, rows, frames, data), type);
    }

    void *symbol() const { return (void*) likely_new; }

public:
    static CallInst *createCall(Builder &builder, Value *type, Value *channels, Value *columns, Value *rows, Value *frames, Value *data)
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> params;
            params.push_back(NativeIntegerType); // type
            params.push_back(NativeIntegerType); // channels
            params.push_back(NativeIntegerType); // columns
            params.push_back(NativeIntegerType); // rows
            params.push_back(NativeIntegerType); // frames
            params.push_back(Type::getInt8PtrTy(C)); // data
            functionType = FunctionType::get(Mat, params, false);
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

        if (argExpr->value()->getType() == Mat)
            return argExpr;

        static FunctionType *functionType = FunctionType::get(Mat, Type::getDoubleTy(C), false);
        Function *likelyScalar = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_scalar", builder.resources->module);
        likelyScalar->setCallingConv(CallingConv::C);
        likelyScalar->setDoesNotAlias(0);

        Immediate result(builder.CreateCall(likelyScalar, builder.cast(argExpr, likely_type_f64)), argExpr->type());
        delete argExpr;
        return new Immediate(result);
    }

    void *symbol() const { return (void*) likely_scalar; }
};
LIKELY_REGISTER(scalar)

class stringExpression : public SimpleUnaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &arg) const
    {
        return new Immediate(createCall(builder, arg), likely_type_i8);
    }

    void *symbol() const { return (void*) likely_string; }

public:
    static CallInst *createCall(Builder &builder, Value *string)
    {
        static FunctionType *functionType = FunctionType::get(Mat, Type::getInt8PtrTy(C), false);
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
    Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &arg) const
    {
        return new Immediate(createCall(builder, arg), arg);
    }

    void *symbol() const { return (void*) likely_copy; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(Mat, Mat, false);
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
    Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &arg) const
    {
        return new Immediate(createCall(builder, arg), arg);
    }

    void *symbol() const { return (void*) likely_retain; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(Mat, Mat, false);
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
    Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &arg) const
    {
        return new Immediate(createCall(builder, arg), arg);
    }

    void *symbol() const { return (void*) likely_release; }

public:
    static CallInst *createCall(Builder &builder, Value *m)
    {
        static FunctionType *functionType = FunctionType::get(Type::getVoidTy(C), Mat, false);
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

struct Kernel : public FunctionExpression
{
    Kernel(Builder &builder, likely_const_ast ast)
        : FunctionExpression(builder, ast) {}

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

    Immediate generate(Builder &builder, const vector<likely_type> &types, string name) const
    {
        if (name.empty())
            name = getUniqueName("kernel");

        for (likely_type type : types)
            if (type == likely_type_null)
                return Immediate(unique_ptr<DynamicFunction>(new DynamicFunction(builder, ast))->generate(builder), likely_type_null);

        Function *function = getKernel(builder, name, types.size(), Mat);
        vector<Immediate> srcs = builder.getArgs(function, types);
        BasicBlock *entry = BasicBlock::Create(C, "entry", function);
        builder.SetInsertPoint(entry);

        Immediate dstChannels = getDimensions(builder, ast, "channels", srcs);
        Immediate dstColumns  = getDimensions(builder, ast, "columns", srcs);
        Immediate dstRows     = getDimensions(builder, ast, "rows", srcs);
        Immediate dstFrames   = getDimensions(builder, ast, "frames", srcs);

        Function *thunk;
        likely_type dstType;
        {
            thunk = getKernel(builder, name + "_thunk", types.size(), Type::getVoidTy(C), Mat, NativeIntegerType, NativeIntegerType);
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
            PHINode *i = builder.CreatePHI(NativeIntegerType, 2, "i");
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
            assert(args->num_atoms == srcs.size());
            for (size_t j=0; j<args->num_atoms; j++)
                builder.define(args->atoms[j]->atom, new kernelArgument(srcs[j], dst, node));

            ManagedExpression result(builder.expression(ast->atoms[2]));
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

            for (size_t i=0; i<args->num_atoms; i++)
                builder.undefine(args->atoms[i]->atom);
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
            static FunctionType *likelyForkType = NULL;
            if (likelyForkType == NULL) {
                vector<Type*> likelyForkParameters;
                likelyForkParameters.push_back(thunk->getType());
                likelyForkParameters.push_back(Type::getInt8Ty(C));
                likelyForkParameters.push_back(NativeIntegerType);
                likelyForkParameters.push_back(Mat);
                Type *likelyForkReturn = Type::getVoidTy(C);
                likelyForkType = FunctionType::get(likelyForkReturn, likelyForkParameters, true);
            }
            Function *likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "likely_fork", builder.resources->module);
            likelyFork->setCallingConv(CallingConv::C);
            likelyFork->setDoesNotCapture(4);
            likelyFork->setDoesNotAlias(4);

            vector<Value*> likelyForkArgs;
            likelyForkArgs.push_back(builder.resources->module->getFunction(thunk->getName()));
            likelyForkArgs.push_back(Builder::constant((double)srcs.size(), 8));
            likelyForkArgs.push_back(kernelSize);
            for (const Immediate &src : srcs)
                likelyForkArgs.push_back(src);
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

        builder.CreateRet(dst);
        return Immediate(function, dstType);
    }

    static Function *getKernel(Builder &builder, const string &name, size_t argc, Type *ret, Type *dst = NULL, Type *start = NULL, Type *stop = NULL)
    {
        Function *kernel;
        switch (argc) {
          case 0: kernel = ::cast<Function>(builder.resources->module->getOrInsertFunction(name, ret, dst, start, stop, NULL)); break;
          case 1: kernel = ::cast<Function>(builder.resources->module->getOrInsertFunction(name, ret, Mat, dst, start, stop, NULL)); break;
          case 2: kernel = ::cast<Function>(builder.resources->module->getOrInsertFunction(name, ret, Mat, Mat, dst, start, stop, NULL)); break;
          case 3: kernel = ::cast<Function>(builder.resources->module->getOrInsertFunction(name, ret, Mat, Mat, Mat, dst, start, stop, NULL)); break;
          default: { kernel = NULL; likely_assert(false, "Kernel::getKernel invalid arity: %d", (int) argc); }
        }
        kernel->addFnAttr(Attribute::NoUnwind);
        kernel->setCallingConv(CallingConv::C);
        if (ret->isPointerTy())
            kernel->setDoesNotAlias(0);
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
        for (size_t i=3; i<ast->num_atoms; i++) {
            if (ast->atoms[i]->is_list && (ast->atoms[i]->num_atoms == 2) && (!ast->atoms[i]->atoms[0]->is_list) && !strcmp(axis, ast->atoms[i]->atoms[0]->atom)) {
                result = builder.cast(unique_ptr<Expression>(builder.expression(ast->atoms[i]->atoms[1])).get(), likely_type_native);
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
    size_t maxParameters() const { return 2; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        return new Kernel(builder, ast);
    }
};
LIKELY_REGISTER(kernel)

struct Lambda : public FunctionExpression
{
    Lambda(Builder &builder, likely_const_ast ast)
        : FunctionExpression(builder, ast) {}

private:
    Immediate generate(Builder &builder, const vector<likely_type> &types, string name) const
    {
        if (name.empty())
            name = getUniqueName("lambda");

        vector<Type*> tys;
        for (likely_type type : types)
            tys.push_back(Builder::ty(type));
        Function *tmpFunction = cast<Function>(builder.resources->module->getOrInsertFunction(name+"_tmp", FunctionType::get(Type::getVoidTy(C), tys, false)));
        vector<Immediate> tmpArgs = builder.getArgs(tmpFunction, types);
        BasicBlock *entry = BasicBlock::Create(C, "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        assert(tmpArgs.size() == ast->atoms[1]->num_atoms);
        for (size_t i=0; i<tmpArgs.size(); i++)
            builder.define(ast->atoms[1]->atoms[i]->atom, tmpArgs[i]);
        ManagedExpression result(builder.expression(ast->atoms[2]));
        if (result.isNull())
            return Immediate(NULL, likely_type_null);
        for (size_t i=0; i<tmpArgs.size(); i++)
            builder.undefine(ast->atoms[1]->atoms[i]->atom);
        builder.CreateRet(result);

        Function *function = cast<Function>(builder.resources->module->getOrInsertFunction(name, FunctionType::get(result.value()->getType(), tys, false)));
        vector<Immediate> args = builder.getArgs(function, types);

        ValueToValueMapTy VMap;
        for (size_t i=0; i<args.size(); i++)
            VMap[tmpArgs[i]] = args[i];

        SmallVector<ReturnInst*, 1> returns;
        CloneFunctionInto(function, tmpFunction, VMap, false, returns);
        tmpFunction->eraseFromParent();
        return Immediate(function, result.type());
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
LIKELY_REGISTER(lambda)

#ifdef LIKELY_IO
#include "likely/likely_io.h"

class printExpression : public Operator, public LibraryFunction
{
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }
    size_t minParameters() const { return 0; }
    Expression *evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        static FunctionType *functionType = FunctionType::get(Mat, Mat, true);
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
            if (rawArg->getType() == Mat) {
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
    Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(Mat, Type::getInt8PtrTy(C), false);
        Function *likelyRead = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_read", builder.resources->module);
        likelyRead->setCallingConv(CallingConv::C);
        likelyRead->setDoesNotAlias(0);
        return new Immediate(builder.CreateCall(likelyRead, arg), likely_type_null);
    }

    void *symbol() const { return (void*) likely_read; }
};
LIKELY_REGISTER(read)

class writeExpression : public SimpleBinaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleBinary(Builder &builder, const ManagedExpression &arg1, const ManagedExpression &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> likelyWriteParameters;
            likelyWriteParameters.push_back(Mat);
            likelyWriteParameters.push_back(Type::getInt8PtrTy(C));
            functionType = FunctionType::get(Mat, likelyWriteParameters, false);
        }
        Function *likelyWrite = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_write", builder.resources->module);
        likelyWrite->setCallingConv(CallingConv::C);
        likelyWrite->setDoesNotAlias(0);
        likelyWrite->setDoesNotAlias(1);
        likelyWrite->setDoesNotCapture(1);
        likelyWrite->setDoesNotAlias(2);
        likelyWrite->setDoesNotCapture(2);
        vector<Value*> likelyWriteArguments;
        likelyWriteArguments.push_back(arg1);
        likelyWriteArguments.push_back(arg2);
        return new Immediate(builder.CreateCall(likelyWrite, likelyWriteArguments), likely_type_null);
    }

    void *symbol() const { return (void*) likely_write; }
};
LIKELY_REGISTER(write)

class decodeExpression : public SimpleUnaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleUnary(Builder &builder, const ManagedExpression &arg) const
    {
        static FunctionType *functionType = FunctionType::get(Mat, Mat, false);
        Function *likelyDecode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_decode", builder.resources->module);
        likelyDecode->setCallingConv(CallingConv::C);
        likelyDecode->setDoesNotAlias(0);
        likelyDecode->setDoesNotAlias(1);
        likelyDecode->setDoesNotCapture(1);
        return new Immediate(builder.CreateCall(likelyDecode, arg), likely_type_null);
    }

    void *symbol() const { return (void*) likely_decode; }
};
LIKELY_REGISTER(decode)

class encodeExpression : public SimpleBinaryOperator, public LibraryFunction
{
    Expression *evaluateSimpleBinary(Builder &builder, const ManagedExpression &arg1, const ManagedExpression &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            vector<Type*> parameters;
            parameters.push_back(Mat);
            parameters.push_back(Type::getInt8PtrTy(C));
            functionType = FunctionType::get(Mat, parameters, false);
        }
        Function *likelyEncode = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_encode", builder.resources->module);
        likelyEncode->setCallingConv(CallingConv::C);
        likelyEncode->setDoesNotAlias(0);
        likelyEncode->setDoesNotAlias(1);
        likelyEncode->setDoesNotCapture(1);
        likelyEncode->setDoesNotAlias(2);
        likelyEncode->setDoesNotCapture(2);
        vector<Value*> likelyEncodeArguments;
        likelyEncodeArguments.push_back(arg1);
        likelyEncodeArguments.push_back(arg2);
        return new Immediate(builder.CreateCall(likelyEncode, likelyEncodeArguments), likely_type_null);
    }

    void *symbol() const { return (void*) likely_encode; }
};
LIKELY_REGISTER(encode)
#endif // LIKELY_IO

} // namespace (anonymous)

likely_env likely_new_env()
{
    return new likely_environment();
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

likely_const_mat likely_dynamic(struct VTable *vTable, likely_const_mat *m)
{
    void *function = NULL;
    for (size_t i=0; i<vTable->functions.size(); i++) {
        const StaticFunction *staticFunction = vTable->functions[i];
        for (likely_arity j=0; j<vTable->n; j++)
            if (m[j]->type != staticFunction->type[j])
                goto Next;
        function = staticFunction->function;
        if (function == NULL)
            return NULL;
        break;
    Next:
        continue;
    }

    if (function == NULL) {
        vector<likely_type> types;
        for (int i=0; i<vTable->n; i++)
            types.push_back(m[i]->type);
        StaticFunction *staticFunction = new StaticFunction(vTable->ast, vTable->env, types, true);
        vTable->functions.push_back(staticFunction);
        function = vTable->functions.back()->function;
    }

    typedef likely_const_mat (*f0)(void);
    typedef likely_const_mat (*f1)(const likely_const_mat);
    typedef likely_const_mat (*f2)(const likely_const_mat, const likely_const_mat);
    typedef likely_const_mat (*f3)(const likely_const_mat, const likely_const_mat, const likely_const_mat);

    likely_const_mat dst;
    switch (vTable->n) {
      case 0: dst = reinterpret_cast<f0>(function)(); break;
      case 1: dst = reinterpret_cast<f1>(function)(m[0]); break;
      case 2: dst = reinterpret_cast<f2>(function)(m[0], m[1]); break;
      case 3: dst = reinterpret_cast<f3>(function)(m[0], m[1], m[2]); break;
      default: dst = NULL; likely_assert(false, "likely_dynamic invalid arity: %d", vTable->n);
    }

    return dst;
}

static map<void*,pair<Resources*,int>> ResourcesLUT;

likely_function likely_compile(likely_const_ast ast, likely_env env)
{
    if (!ast || !env) return NULL;
    Resources *r = new Resources(true, true);
    Builder builder(r, env);
    DynamicFunction *df = static_cast<DynamicFunction*>(builder.expression(ast));
    likely_function f = df ? reinterpret_cast<likely_function>(r->finalize(df->generate(builder))) : NULL;
    if (f) ResourcesLUT[(void*)f] = pair<Resources*,int>(r, 1);
    else   delete r;
    return f;
}

void *likely_retain_function(void *function)
{
    if (function) ResourcesLUT[function].second++;
    return function;
}

void likely_release_function(void *function)
{
    if (!function) return;
    pair<Resources*,int> &df = ResourcesLUT[function];
    if (--df.second) return;
    ResourcesLUT.erase(function);
    delete df.first;
}

void likely_compile_to_file(likely_const_ast ast, likely_env env, const char *symbol_name, likely_type *types, likely_arity n, const char *file_name, bool native)
{
    if (!ast || !env) return;
    StaticFunction(ast, env, vector<likely_type>(types, types+n), native, symbol_name).write(file_name);
}

likely_mat likely_eval(likely_const_ast ast, likely_env env)
{
    if (!ast || !env) return NULL;
    likely_const_ast expr = likely_ast_from_string("(lambda () (scalar <ast>))");
    expr->atoms[2]->atoms[1] = likely_retain_ast(ast);
    StaticFunction staticFunction(expr, env, vector<likely_type>(), true);
    likely_release_ast(expr);
    if (staticFunction.function) return reinterpret_cast<likely_mat(*)(void)>(staticFunction.function)();
    else                         return NULL;
}
