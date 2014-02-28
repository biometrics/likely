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
static PointerType *Matrix = NULL;
static LLVMContext &C = getGlobalContext();

class Builder;
struct Expression
{
    virtual ~Expression() {}
    virtual Value *value() const = 0;
    virtual likely_type type() const = 0;
    virtual Expression *evaluate(Builder &builder, likely_ast ast) const = 0;

    operator Value*() const { return value(); }
    operator likely_type() const { return type(); }

    Value *take()
    {
        Value *result = value();
        delete this; // With great power comes great responsibility
        return result;
    }
};

} // namespace (anonymous)

struct likely_env_struct : public map<string,stack<shared_ptr<Expression>>>
{
    static likely_env_struct defaultExprs;
    mutable int ref_count = 1;
    likely_env_struct(const likely_env_struct &exprs = defaultExprs)
        : map<string,stack<shared_ptr<Expression>>>(exprs) {}
};
likely_env_struct likely_env_struct::defaultExprs;

namespace {

static Expression *likelyThrow(likely_ast ast, const char *message)
{
    likely_throw(ast, message);
    return NULL;
}

static Expression *likelyThrowArgumentCount(likely_ast ast, const string &function, int parameters, int arguments)
{
    stringstream stream;
    stream << function << " with: " << parameters << " parameters passed: " << arguments << " arguments";
    return likelyThrow(ast, stream.str().c_str());
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
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        (void) builder;
        (void) ast;
        return new Immediate(value(), type());
    }
};

class Builder : public IRBuilder<>
{
    likely_env env;

public:
    Module *module;
    string name;

    Builder(Module *module, likely_env env, const string &name)
        : IRBuilder<>(C), env(likely_retain_env(env)), module(module), name(name)
    {}

    Builder(Builder &other, likely_env env)
        : Builder(other.module, env, other.name) {}

    ~Builder() { likely_release_env(env); }

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

    Immediate data    (const Expression *matrix) { return Immediate(CreatePointerCast(CreateLoad(CreateStructGEP(*matrix, 1), "data"), ty(*matrix, true)), matrix->type() & likely_type_mask); }
    Immediate channels(const Expression *matrix) { return likely_multi_channel(*matrix) ? Immediate(CreateLoad(CreateStructGEP(*matrix, 2), "channels"), likely_type_native) : one(); }
    Immediate columns (const Expression *matrix) { return likely_multi_column (*matrix) ? Immediate(CreateLoad(CreateStructGEP(*matrix, 3), "columns" ), likely_type_native) : one(); }
    Immediate rows    (const Expression *matrix) { return likely_multi_row    (*matrix) ? Immediate(CreateLoad(CreateStructGEP(*matrix, 4), "rows"    ), likely_type_native) : one(); }
    Immediate frames  (const Expression *matrix) { return likely_multi_frame  (*matrix) ? Immediate(CreateLoad(CreateStructGEP(*matrix, 5), "frames"  ), likely_type_native) : one(); }

    void steps(const Expression *matrix, Value **columnStep, Value **rowStep, Value **frameStep)
    {
        *columnStep = channels(matrix);
        *rowStep    = CreateMul(columns(matrix), *columnStep, "y_step");
        *frameStep  = CreateMul(rows(matrix), *rowStep, "t_step");
    }

    Immediate cast(const Expression *x, likely_type type)
    {
        if ((x->type() & likely_type_mask) == (type & likely_type_mask))
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

    likely_env snapshot() const { return new likely_env_struct(*env); }
};

class ManagedExpression : public Expression
{
    Expression *e;

public:
    ManagedExpression(Expression *e) : e(e) {}
    ~ManagedExpression() { delete e; }
    bool isNull() const { return !e; }
    Value* value() const { return e->value(); }
    likely_type type() const { return e->type(); }
    Expression *evaluate(Builder &builder, likely_ast ast) const { return e->evaluate(builder, ast); }
};

#define TRY_EXPR(BUILDER, AST, EXPR)                    \
const ManagedExpression EXPR(expression(BUILDER, AST)); \
if (EXPR.isNull()) return NULL;                         \

struct Operator : public Expression
{
    static Expression *expression(Builder &builder, likely_ast ast)
    {
        if (ast->is_list) {
            if (ast->num_atoms == 0)
                return likelyThrow(ast, "Empty expression");
            likely_ast op = ast->atoms[0];
            if (!op->is_list)
                if (Expression *e = builder.lookup(op->atom))
                    return e->evaluate(builder, ast);
            TRY_EXPR(builder, op, e);
            return e.evaluate(builder, ast);
        }
        const string op = ast->atom;

        if (Expression *e = builder.lookup(op))
            return e->evaluate(builder, ast);

        if ((op.front() == '"') && (op.back() == '"'))
            return new Immediate(builder.CreateGlobalStringPtr(op.substr(1, op.length()-2)), likely_type_u8);

        { // Is it a number?
            char *p;
            const double value = strtod(op.c_str(), &p);
            if (*p == 0)
                return new Immediate(Builder::constant(value, likely_type_from_value(value)));
        }

        likely_type type = likely_type_from_string(op.c_str());
        if (type != likely_type_null)
            return new Immediate(Builder::constant(type, likely_type_u32));

        return likelyThrow(ast, "unrecognized literal");
    }

private:
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
};

template <class T>
struct RegisterExpression
{
    RegisterExpression(const string &symbol)
    {
        likely_env_struct::defaultExprs[symbol].push(shared_ptr<Expression>(new T()));
    }
};
#define LIKELY_REGISTER_EXPRESSION(EXP, SYM) static struct RegisterExpression<EXP##Expression> Register##EXP##Expression(SYM);
#define LIKELY_REGISTER(EXP) LIKELY_REGISTER_EXPRESSION(EXP, #EXP)

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
LIKELY_REGISTER_TYPE(mask)
LIKELY_REGISTER_TYPE(parallel)
LIKELY_REGISTER_TYPE(heterogeneous)
LIKELY_REGISTER_TYPE(multi_channel)
LIKELY_REGISTER_TYPE(multi_column)
LIKELY_REGISTER_TYPE(multi_row)
LIKELY_REGISTER_TYPE(multi_frame)
LIKELY_REGISTER_TYPE(multi_dimension)
LIKELY_REGISTER_TYPE(saturation)
LIKELY_REGISTER_TYPE(reserved)

class UnaryOperator : public Operator
{
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        if (!ast->is_list || (ast->num_atoms != 2))
            return likelyThrow(ast, "expected 1 operand");
        return evaluateUnary(builder, ast->atoms[1]);
    }
    virtual Expression *evaluateUnary(Builder &builder, likely_ast arg) const = 0;
};

class notExpression : public UnaryOperator
{
    Expression *evaluateUnary(Builder &builder, likely_ast arg) const
    {
        TRY_EXPR(builder, arg, argExpr)
        return new Immediate(builder.CreateXor(builder.intMax(argExpr), argExpr.value()), argExpr);
    }
};
LIKELY_REGISTER_EXPRESSION(not, "~")

class typeExpression : public UnaryOperator
{
    Expression *evaluateUnary(Builder &builder, likely_ast arg) const
    {
        TRY_EXPR(builder, arg, argExpr)
        return new Immediate(Builder::type(argExpr));
    }
};
LIKELY_REGISTER(type)

class scalarExpression : public UnaryOperator
{
    Expression *evaluateUnary(Builder &builder, likely_ast arg) const
    {
        Expression *argExpr = expression(builder, arg);
        if (!argExpr)
            return new Immediate(builder.zero());

        if (argExpr->value()->getType() == Matrix)
            return argExpr;

        static FunctionType* LikelyScalarSignature = NULL;
        if (LikelyScalarSignature == NULL)
            LikelyScalarSignature = FunctionType::get(Matrix, Type::getDoubleTy(C), false);

        Function *likelyScalar = Function::Create(LikelyScalarSignature, GlobalValue::ExternalLinkage, "likely_scalar", builder.module);
        likelyScalar->setCallingConv(CallingConv::C);
        likelyScalar->setDoesNotAlias(0);

         // An impossible case used to ensure that `likely_scalar` isn't stripped when optimizing executable size
        if (likelyScalar == NULL)
            likely_scalar(0);

        Immediate result(builder.CreateCall(likelyScalar, builder.cast(argExpr, likely_type_f64)), argExpr->type());
        delete argExpr;
        return new Immediate(result);
    }
};
LIKELY_REGISTER(scalar)

class UnaryMathOperator : public UnaryOperator
{
    Expression *evaluateUnary(Builder &builder, likely_ast arg) const
    {
        TRY_EXPR(builder, arg, x)
        Immediate xc(builder.cast(&x, Builder::validFloatType(x)));
        vector<Type*> args;
        args.push_back(xc.value_->getType());
        return new Immediate(builder.CreateCall(Intrinsic::getDeclaration(builder.module, id(), args), xc), xc);
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

class BinaryOperator : public Operator
{
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        if (!ast->is_list || (ast->num_atoms != 3))
            return likelyThrow(ast, "expected 2 operands");
        return evaluateBinary(builder, ast->atoms[1], ast->atoms[2]);
    }
    virtual Expression *evaluateBinary(Builder &builder, likely_ast arg1, likely_ast arg2) const = 0;
};

class castExpression : public BinaryOperator
{
    Expression *evaluateBinary(Builder &builder, likely_ast arg1, likely_ast arg2) const
    {
        TRY_EXPR(builder, arg1, x)
        TRY_EXPR(builder, arg2, type)
        return new Immediate(builder.cast(&x, (likely_type)LLVM_VALUE_TO_INT(type.value())));
    }
};
LIKELY_REGISTER(cast)

class ArithmeticOperator : public BinaryOperator
{
    Expression *evaluateBinary(Builder &builder, likely_ast arg1, likely_ast arg2) const
    {
        TRY_EXPR(builder, arg1, lhs)
        TRY_EXPR(builder, arg2, rhs)
        likely_type type = likely_type_from_types(lhs, rhs);
        Immediate lhsc(builder.cast(&lhs, type));
        Immediate rhsc(builder.cast(&rhs, type));
        return evaluateArithmetic(builder, &lhsc, &rhsc, type);
    }
    virtual Expression *evaluateArithmetic(Builder &builder, const Expression *lhs, const Expression *rhs, likely_type type) const = 0;
};

class addExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Expression *lhs, const Expression *rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return new Immediate(builder.CreateFAdd(*lhs, *rhs), type);
        } else {
            if (likely_saturation(type)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module, likely_signed(type) ? Intrinsic::sadd_with_overflow : Intrinsic::uadd_with_overflow, lhs->value()->getType()), *lhs, *rhs);
                Value *overflowResult = likely_signed(type) ? builder.CreateSelect(builder.CreateICmpSGE(*lhs, Builder::zero(type)), Builder::intMax(type), Builder::intMin(type)) : Builder::intMax(type);
                return new Immediate(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), type);
            } else {
                return new Immediate(builder.CreateAdd(*lhs, *rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(add, "+")

class subtractExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Expression *lhs, const Expression *rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return new Immediate(builder.CreateFSub(*lhs, *rhs), type);
        } else {
            if (likely_saturation(type)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module, likely_signed(type) ? Intrinsic::ssub_with_overflow : Intrinsic::usub_with_overflow, lhs->value()->getType()), lhs->value(), rhs->value());
                Value *overflowResult = likely_signed(type) ? builder.CreateSelect(builder.CreateICmpSGE(lhs->value(), Builder::zero(type)), Builder::intMax(type), Builder::intMin(type)) : Builder::intMin(type);
                return new Immediate(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), type);
            } else {
                return new Immediate(builder.CreateSub(*lhs, *rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(subtract, "-")

class multiplyExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Expression *lhs, const Expression *rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return new Immediate(builder.CreateFMul(*lhs, *rhs), type);
        } else {
            if (likely_saturation(type)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module, likely_signed(type) ? Intrinsic::smul_with_overflow : Intrinsic::umul_with_overflow, lhs->value()->getType()), lhs->value(), rhs->value());
                Value *zero = Builder::zero(type);
                Value *overflowResult = likely_signed(type) ? builder.CreateSelect(builder.CreateXor(builder.CreateICmpSGE(lhs->value(), zero), builder.CreateICmpSGE(rhs->value(), zero)), Builder::intMin(type), Builder::intMax(type)) : Builder::intMax(type);
                return new Immediate(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), type);
            } else {
                return new Immediate(builder.CreateMul(*lhs, *rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(multiply, "*")

class divideExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Expression *n, const Expression *d, likely_type type) const
    {
        if (likely_floating(type)) {
            return new Immediate(builder.CreateFDiv(*n, *d), type);
        } else {
            if (likely_signed(type)) {
                if (likely_saturation(type)) {
                    Value *safe_i = builder.CreateAdd(*n, builder.CreateZExt(builder.CreateICmpNE(builder.CreateOr(builder.CreateAdd(d->value(), Builder::one(type)), builder.CreateAdd(n->value(), Builder::intMin(type))), Builder::zero(type)), n->value()->getType()));
                    return new Immediate(builder.CreateSDiv(safe_i, *d), type);
                } else {
                    return new Immediate(builder.CreateSDiv(*n, *d), type);
                }
            } else {
                return new Immediate(builder.CreateUDiv(*n, *d), type);
            }
        }
    }
};
LIKELY_REGISTER_EXPRESSION(divide, "/")

class remExpression : public ArithmeticOperator
{
    Expression *evaluateArithmetic(Builder &builder, const Expression *lhs, const Expression *rhs, likely_type type) const
    {
        return new Immediate(likely_floating(type) ? builder.CreateFRem(*lhs, *rhs)
                                                   : (likely_signed(type) ? builder.CreateSRem(*lhs, *rhs)
                                                                          : builder.CreateURem(*lhs, *rhs)), type);
    }
};
LIKELY_REGISTER_EXPRESSION(rem, "%")

#define LIKELY_REGISTER_LOGIC(OP, SYM)                                                                                     \
class OP##Expression : public ArithmeticOperator                                                                           \
{                                                                                                                          \
    Expression *evaluateArithmetic(Builder &builder, const Expression *lhs, const Expression *rhs, likely_type type) const \
    {                                                                                                                      \
        return new Immediate(builder.Create##OP(*lhs, rhs->value()), type);                                                \
    }                                                                                                                      \
};                                                                                                                         \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                        \

LIKELY_REGISTER_LOGIC(And, "and")
LIKELY_REGISTER_LOGIC(Or, "or")
LIKELY_REGISTER_LOGIC(Xor, "xor")
LIKELY_REGISTER_LOGIC(Shl, "shl")
LIKELY_REGISTER_LOGIC(LShr, "lshr")
LIKELY_REGISTER_LOGIC(AShr, "ashr")

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                \
class OP##Expression : public ArithmeticOperator                                                                           \
{                                                                                                                          \
    Expression *evaluateArithmetic(Builder &builder, const Expression *lhs, const Expression *rhs, likely_type type) const \
    {                                                                                                                      \
        return new Immediate(likely_floating(type) ? builder.CreateFCmpO##OP(*lhs, *rhs)                                   \
                                                   : (likely_signed(type) ? builder.CreateICmpS##OP(*lhs, *rhs)            \
                                                                          : builder.CreateICmpU##OP(*lhs, *rhs)), type);   \
    }                                                                                                                      \
};                                                                                                                         \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                        \

LIKELY_REGISTER_COMPARISON(LT, "<")
LIKELY_REGISTER_COMPARISON(LE, "<=")
LIKELY_REGISTER_COMPARISON(GT, ">")
LIKELY_REGISTER_COMPARISON(GE, ">=")

#define LIKELY_REGISTER_EQUALITY(OP, SYM)                                                                                  \
class OP##Expression : public ArithmeticOperator                                                                           \
{                                                                                                                          \
    Expression *evaluateArithmetic(Builder &builder, const Expression *lhs, const Expression *rhs, likely_type type) const \
    {                                                                                                                      \
        return new Immediate(likely_floating(type) ? builder.CreateFCmpO##OP(*lhs, *rhs)                                   \
                                                   : builder.CreateICmp##OP(*lhs, *rhs), type);                            \
    }                                                                                                                      \
};                                                                                                                         \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                        \

LIKELY_REGISTER_EQUALITY(EQ, "==")
LIKELY_REGISTER_EQUALITY(NE, "!=")

class BinaryMathOperator : public BinaryOperator
{
    Expression *evaluateBinary(Builder &builder, likely_ast arg1, likely_ast arg2) const
    {
        TRY_EXPR(builder, arg1, x)
        TRY_EXPR(builder, arg2, n)
        const likely_type type = nIsInteger() ? x : likely_type_from_types(x, n);
        Immediate xc(builder.cast(&x, Builder::validFloatType(type)));
        Immediate nc(builder.cast(&n, nIsInteger() ? likely_type_i32 : xc));
        vector<Type*> args;
        args.push_back(xc.value_->getType());
        return new Immediate(builder.CreateCall2(Intrinsic::getDeclaration(builder.module, id(), args), xc, nc), xc);
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

class TernaryOperator : public Operator
{
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        if (!ast->is_list || (ast->num_atoms != 4))
            return likelyThrow(ast, "expected 3 operands");
        return evaluateTernary(builder, ast->atoms[1], ast->atoms[2], ast->atoms[3]);
    }
    virtual Expression *evaluateTernary(Builder &builder, likely_ast arg1, likely_ast arg2, likely_ast arg3) const = 0;
};

class fmaExpression : public TernaryOperator
{
    Expression *evaluateTernary(Builder &builder, likely_ast arg1, likely_ast arg2, likely_ast arg3) const
    {
        TRY_EXPR(builder, arg1, a)
        TRY_EXPR(builder, arg2, b)
        TRY_EXPR(builder, arg3, c)
        const likely_type type = likely_type_from_types(likely_type_from_types(a, b), c);
        Immediate ac(builder.cast(&a, Builder::validFloatType(type)));
        Immediate bc(builder.cast(&b, ac));
        Immediate cc(builder.cast(&c, ac));
        vector<Type*> args;
        args.push_back(ac.value_->getType());
        return new Immediate(builder.CreateCall3(Intrinsic::getDeclaration(builder.module, Intrinsic::fma, args), ac, bc, cc), ac);
    }
};
LIKELY_REGISTER(fma)

class selectExpression : public TernaryOperator
{
    Expression *evaluateTernary(Builder &builder, likely_ast arg1, likely_ast arg2, likely_ast arg3) const
    {
        TRY_EXPR(builder, arg1, c)
        TRY_EXPR(builder, arg2, t)
        TRY_EXPR(builder, arg3, f)
        const likely_type type = likely_type_from_types(t, f);
        return new Immediate(builder.CreateSelect(c, builder.cast(&t, type), builder.cast(&f, type)), type);
    }
};
LIKELY_REGISTER(select)

class defineExpression : public Operator
{
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        if (!ast->is_list || (ast->num_atoms != 3))
            return likelyThrowArgumentCount(ast, "define", 3, ast->is_list ? ast->num_atoms : 0);
        if (ast->atoms[1]->is_list)
            return likelyThrow(ast->atoms[1], "define expected an atom name");
        builder.define(ast->atoms[1]->atom, expression(builder, ast->atoms[2]));
        return NULL;
    }
};
LIKELY_REGISTER(define)

class FunctionExpression : public Operator
{
    likely_env env;

protected:
    likely_ast ast;

public:
    FunctionExpression(Builder &builder, likely_ast ast)
        : env(builder.snapshot()), ast(likely_retain_ast(ast)) {}

    ~FunctionExpression()
    {
        likely_release_ast(ast);
        likely_release_env(env);
    }

    virtual Immediate generate(Builder &builder, const vector<likely_type> &types) const = 0;

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

private:
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        assert(ast->is_list);
        const int parameters = argc();
        const int arguments = ast->num_atoms - 1;
        if (parameters != arguments)
            return likelyThrowArgumentCount(ast, "lambda", parameters, arguments);

        vector<Value*> args;
        vector<likely_type> types;
        for (int i=0; i<arguments; i++) {
            TRY_EXPR(builder, ast->atoms[i+1], arg)
            args.push_back(arg);
            types.push_back(arg);
        }

        Builder lambdaBuilder(builder, env);
        Immediate i = generate(lambdaBuilder, types);
        Function *f = cast<Function>(i.value_);
        if (f) return new Immediate(builder.CreateCall(f, args), i.type_);
        else   return NULL;
    }
};

struct Kernel : public FunctionExpression
{
    Kernel(Builder &builder, likely_ast ast)
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
        Expression *evaluate(Builder &builder, likely_ast ast) const
        {
            if (ast->is_list)
                return likelyThrow(ast, "kernel argument does not take arguments");

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

    Immediate generate(Builder &builder, const vector<likely_type> &types) const
    {
        Function *function = getKernel(builder, types.size(), Matrix);
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
            thunk = getKernel(builder, types.size(), Type::getVoidTy(C), Matrix, NativeIntegerType, NativeIntegerType);
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

            const likely_ast args = ast->atoms[1];
            assert(args->num_atoms == srcs.size());
            for (size_t j=0; j<args->num_atoms; j++)
                builder.define(args->atoms[j]->atom, new kernelArgument(srcs[j], dst, node));

            ManagedExpression result(expression(builder, ast->atoms[2]));
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

        static FunctionType* LikelyNewSignature = NULL;
        if (LikelyNewSignature == NULL) {
            vector<Type*> newParameters;
            newParameters.push_back(Type::getInt32Ty(C)); // type
            newParameters.push_back(NativeIntegerType); // channels
            newParameters.push_back(NativeIntegerType); // columns
            newParameters.push_back(NativeIntegerType); // rows
            newParameters.push_back(NativeIntegerType); // frames
            newParameters.push_back(Type::getInt8PtrTy(C)); // data
            newParameters.push_back(Type::getInt8Ty(C)); // copy
            LikelyNewSignature = FunctionType::get(Matrix, newParameters, false);
        }
        Function *likelyNew = Function::Create(LikelyNewSignature, GlobalValue::ExternalLinkage, "likely_new", builder.module);
        likelyNew->setCallingConv(CallingConv::C);
        likelyNew->setDoesNotAlias(0);
        likelyNew->setDoesNotAlias(6);
        likelyNew->setDoesNotCapture(6);

        std::vector<Value*> likelyNewArgs;
        likelyNewArgs.push_back(Builder::type(dstType));
        likelyNewArgs.push_back(dstChannels);
        likelyNewArgs.push_back(dstColumns);
        likelyNewArgs.push_back(dstRows);
        likelyNewArgs.push_back(dstFrames);
        likelyNewArgs.push_back(ConstantPointerNull::get(Type::getInt8PtrTy(C)));
        likelyNewArgs.push_back(builder.constant(0, 8));
        Value *dst = builder.CreateCall(likelyNew, likelyNewArgs);

        // An impossible case used to ensure that `likely_new` isn't stripped when optimizing executable size
        if (likelyNew == NULL)
            likely_new(likely_type_null, 0, 0, 0, 0, NULL, 0);

        Value *kernelSize = builder.CreateMul(builder.CreateMul(builder.CreateMul(dstChannels, dstColumns), dstRows), dstFrames);

        if (!srcs.empty() && likely_parallel(srcs[0])) {
            static FunctionType *likelyForkType = NULL;
            if (likelyForkType == NULL) {
                vector<Type*> likelyForkParameters;
                likelyForkParameters.push_back(thunk->getType());
                likelyForkParameters.push_back(Type::getInt8Ty(C));
                likelyForkParameters.push_back(NativeIntegerType);
                likelyForkParameters.push_back(Matrix);
                Type *likelyForkReturn = Type::getVoidTy(C);
                likelyForkType = FunctionType::get(likelyForkReturn, likelyForkParameters, true);
            }
            Function *likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "likely_fork", builder.module);
            likelyFork->setCallingConv(CallingConv::C);
            likelyFork->setDoesNotCapture(4);
            likelyFork->setDoesNotAlias(4);

            vector<Value*> likelyForkArgs;
            likelyForkArgs.push_back(builder.module->getFunction(thunk->getName()));
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

    static Function *getKernel(Builder &builder, size_t argc, Type *ret, Type *dst = NULL, Type *start = NULL, Type *stop = NULL)
    {
        Function *kernel;
        const string name = builder.name + (dst == NULL ? "" : "_thunk");
        switch (argc) {
          case 0: kernel = ::cast<Function>(builder.module->getOrInsertFunction(name, ret, dst, start, stop, NULL)); break;
          case 1: kernel = ::cast<Function>(builder.module->getOrInsertFunction(name, ret, Matrix, dst, start, stop, NULL)); break;
          case 2: kernel = ::cast<Function>(builder.module->getOrInsertFunction(name, ret, Matrix, Matrix, dst, start, stop, NULL)); break;
          case 3: kernel = ::cast<Function>(builder.module->getOrInsertFunction(name, ret, Matrix, Matrix, Matrix, dst, start, stop, NULL)); break;
          default: { kernel = NULL; likely_assert(false, "Kernel::getKernel invalid arity: %zu", argc); }
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

    static Immediate getDimensions(Builder &builder, likely_ast ast, const char *axis, const vector<Immediate> &srcs)
    {
        Value *result = NULL;

        // Look for a dimensionality expression
        for (size_t i=3; i<ast->num_atoms; i++) {
            if (ast->atoms[i]->is_list && (ast->atoms[i]->num_atoms == 2) && (!ast->atoms[i]->atoms[0]->is_list) && !strcmp(axis, ast->atoms[i]->atoms[0]->atom)) {
                result = builder.cast(unique_ptr<Expression>(expression(builder, ast->atoms[i]->atoms[1])).get(), likely_type_native);
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
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        return new Kernel(builder, ast);
    }
};
LIKELY_REGISTER(kernel)

struct Lambda : public FunctionExpression
{
    Lambda(Builder &builder, likely_ast ast)
        : FunctionExpression(builder, ast) {}

private:
    Immediate generate(Builder &builder, const vector<likely_type> &types) const
    {
        vector<Type*> tys;
        for (likely_type type : types)
            tys.push_back(Builder::ty(type));
        Function *tmpFunction = cast<Function>(builder.module->getOrInsertFunction(builder.name+"_tmp", FunctionType::get(Type::getVoidTy(C), tys, false)));
        vector<Immediate> tmpArgs = builder.getArgs(tmpFunction, types);
        BasicBlock *entry = BasicBlock::Create(C, "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        assert(tmpArgs.size() == ast->atoms[1]->num_atoms);
        for (size_t i=0; i<tmpArgs.size(); i++)
            builder.define(ast->atoms[1]->atoms[i]->atom, tmpArgs[i]);
        ManagedExpression result(expression(builder, ast->atoms[2]));
        for (size_t i=0; i<tmpArgs.size(); i++)
            builder.undefine(ast->atoms[1]->atoms[i]->atom);
        builder.CreateRet(result);

        Function *function = cast<Function>(builder.module->getOrInsertFunction(builder.name, FunctionType::get(result.value()->getType(), tys, false)));
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
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        return new Lambda(builder, ast);
    }
};
LIKELY_REGISTER(lambda)

#ifdef LIKELY_IO
#include "likely/likely_io.h"

class readExpression : public Operator
{
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        static FunctionType *LikelyReadSignature = NULL;
        if (LikelyReadSignature == NULL) {
            LikelyReadSignature = FunctionType::get(Matrix, Type::getInt8PtrTy(C), false);
            // An impossible case used to ensure that `likely_read` isn't stripped when optimizing executable size
            if (LikelyReadSignature == NULL)
                likely_read(NULL);
        }
        Function *likelyRead = Function::Create(LikelyReadSignature, GlobalValue::ExternalLinkage, "likely_read", builder.module);
        likelyRead->setCallingConv(CallingConv::C);
        likelyRead->setDoesNotAlias(0);
        return new Immediate(builder.CreateCall(likelyRead, expression(builder, ast->atoms[1])->take()), likely_type_null);
    }
};
LIKELY_REGISTER(read)

class writeExpression : public Operator
{
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        static FunctionType *LikelyWriteSignature = NULL;
        if (LikelyWriteSignature == NULL) {
            vector<Type*> likelyWriteParameters;
            likelyWriteParameters.push_back(Matrix);
            likelyWriteParameters.push_back(Type::getInt8PtrTy(C));
            LikelyWriteSignature = FunctionType::get(Matrix, likelyWriteParameters, false);
            // An impossible case used to ensure that `likely_write` isn't stripped when optimizing executable size
            if (LikelyWriteSignature == NULL)
                likely_write(NULL, NULL);
        }
        Function *likelyWrite = Function::Create(LikelyWriteSignature, GlobalValue::ExternalLinkage, "likely_write", builder.module);
        likelyWrite->setCallingConv(CallingConv::C);
        likelyWrite->setDoesNotAlias(0);
        likelyWrite->setDoesNotAlias(1);
        likelyWrite->setDoesNotCapture(1);
        likelyWrite->setDoesNotAlias(2);
        likelyWrite->setDoesNotCapture(2);
        vector<Value*> likelyWriteArguments;
        likelyWriteArguments.push_back(expression(builder, ast->atoms[1])->take());
        likelyWriteArguments.push_back(expression(builder, ast->atoms[2])->take());
        return new Immediate(builder.CreateCall(likelyWrite, likelyWriteArguments), likely_type_null);
    }
};
LIKELY_REGISTER(write)

class decodeExpression : public Operator
{
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        static FunctionType *LikelyDecodeSignature = NULL;
        if (LikelyDecodeSignature == NULL) {
            LikelyDecodeSignature = FunctionType::get(Matrix, Matrix, false);
            // An impossible case used to ensure that `likely_decode` isn't stripped when optimizing executable size
            if (LikelyDecodeSignature == NULL)
                likely_decode(NULL);
        }
        Function *likelyDecode = Function::Create(LikelyDecodeSignature, GlobalValue::ExternalLinkage, "likely_decode", builder.module);
        likelyDecode->setCallingConv(CallingConv::C);
        likelyDecode->setDoesNotAlias(0);
        likelyDecode->setDoesNotAlias(1);
        likelyDecode->setDoesNotCapture(1);
        return new Immediate(builder.CreateCall(likelyDecode, expression(builder, ast->atoms[1])->take()), likely_type_null);
    }
};
LIKELY_REGISTER(decode)

class encodeExpression : public Operator
{
    Expression *evaluate(Builder &builder, likely_ast ast) const
    {
        static FunctionType *LikelyEncodeSignature = NULL;
        if (LikelyEncodeSignature == NULL) {
            vector<Type*> likelyEncodeParameters;
            likelyEncodeParameters.push_back(Matrix);
            likelyEncodeParameters.push_back(Type::getInt8PtrTy(C));
            LikelyEncodeSignature = FunctionType::get(Matrix, likelyEncodeParameters, false);
            // An impossible case used to ensure that `likely_encode` isn't stripped when optimizing executable size
            if (LikelyEncodeSignature == NULL)
                likely_encode(NULL, NULL);
        }
        Function *likelyEncode = Function::Create(LikelyEncodeSignature, GlobalValue::ExternalLinkage, "likely_encode", builder.module);
        likelyEncode->setCallingConv(CallingConv::C);
        likelyEncode->setDoesNotAlias(0);
        likelyEncode->setDoesNotAlias(1);
        likelyEncode->setDoesNotCapture(1);
        likelyEncode->setDoesNotAlias(2);
        likelyEncode->setDoesNotCapture(2);
        vector<Value*> likelyEncodeArguments;
        likelyEncodeArguments.push_back(expression(builder, ast->atoms[1])->take());
        likelyEncodeArguments.push_back(expression(builder, ast->atoms[2])->take());
        return new Immediate(builder.CreateCall(likelyEncode, likelyEncodeArguments), likely_type_null);
    }
};
LIKELY_REGISTER(encode)
#endif // LIKELY_IO

struct JITResources
{
    string name;
    Module *module;
    ExecutionEngine *executionEngine;
    TargetMachine *targetMachine;

    JITResources(bool native, const string &symbol_name = string())
        : name(symbol_name)
    {
        if (Matrix == NULL) {
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
            Matrix = PointerType::getUnqual(StructType::create("likely_matrix_struct",
                                                               PointerType::getUnqual(StructType::create(C, "likely_matrix_private")), // d_ptr
                                                               Type::getInt8PtrTy(C), // data
                                                               NativeIntegerType,     // channels
                                                               NativeIntegerType,     // columns
                                                               NativeIntegerType,     // rows
                                                               NativeIntegerType,     // frames
                                                               Type::getInt32Ty(C),   // type
                                                               NULL));
        }

        const bool JIT = symbol_name.empty();

        if (JIT) {
            static int index = 0;
            stringstream stream; stream << "likely_" << index++;
            name = stream.str();
        }

        module = new Module(name, C);
        likely_assert(module != NULL, "failed to create module");

        if (native) {
            string targetTriple = sys::getProcessTriple();
#ifdef _WIN32
            if (JIT)
                targetTriple = targetTriple + "-elf";
#endif // _WIN32
            module->setTargetTriple(Triple::normalize(targetTriple));
        }

        string error;
        EngineBuilder engineBuilder(module);
        engineBuilder.setMCPU(sys::getHostCPUName())
                     .setEngineKind(EngineKind::JIT)
                     .setOptLevel(CodeGenOpt::Aggressive)
                     .setErrorStr(&error)
                     .setUseMCJIT(true);

        if (JIT) {
            executionEngine = engineBuilder.create();
            likely_assert(executionEngine != NULL, "failed to create execution engine with error: %s", error.c_str());
        } else {
            executionEngine = NULL;
        }

        if (native) {
            engineBuilder.setCodeModel(CodeModel::Default);
            targetMachine = engineBuilder.selectTarget();
            likely_assert(targetMachine != NULL, "failed to create target machine with error: %s", error.c_str());
        } else {
            targetMachine = NULL;
        }
    }

    ~JITResources()
    {
        delete targetMachine;
        if (executionEngine) delete executionEngine; // owns module
        else                 delete module;
    }

    void *finalize(Function *function)
    {
        if (!function)
            return NULL;

        if (targetMachine) {
            static PassManager *PM = NULL;
            if (!PM) {
                PM = new PassManager();
                PM->add(createVerifierPass());
                PM->add(new TargetLibraryInfo(Triple(module->getTargetTriple())));
                PM->add(new DataLayout(module));
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
            return executionEngine->getPointerToFunction(function);
        } else {
            return NULL;
        }
    }
};

struct FunctionBuilder : private JITResources
{
    likely_type *type;
    void *function;

    FunctionBuilder(likely_ast ast, likely_env env, const vector<likely_type> &types, bool native, const string &symbol_name = string())
        : JITResources(native, symbol_name)
    {
        likely_assert(ast->is_list && (ast->num_atoms > 0) && !ast->atoms[0]->is_list &&
                      (!strcmp(ast->atoms[0]->atom, "lambda") || !strcmp(ast->atoms[0]->atom, "kernel")),
                      "expected a lambda/kernel expression");
        type = new likely_type[types.size()];
        memcpy(type, types.data(), sizeof(likely_type) * types.size());
        Builder builder(module, env, name);
        unique_ptr<Expression> result(Operator::expression(builder, ast));
        function = finalize(dyn_cast<Function>(static_cast<FunctionExpression*>(result.get())->generate(builder, types).value_));
    }

    ~FunctionBuilder()
    {
        delete[] type;
    }

    void write(const string &fileName) const
    {
        const string extension = fileName.substr(fileName.find_last_of(".") + 1);

        string errorInfo;
        tool_output_file output(fileName.c_str(), errorInfo);
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

struct VTable : public JITResources
{
    static PointerType *vtableType;
    likely_ast ast;
    likely_env env;
    likely_arity n;
    vector<FunctionBuilder*> functions;
    Function *likelyDispatch;

    VTable(likely_ast ast, likely_env env)
        : JITResources(true), ast(likely_retain_ast(ast)), env(likely_retain_env(env))
    {
        // Try to compute arity
        if (ast->is_list && (ast->num_atoms > 1))
            if (ast->atoms[1]->is_list) n = ast->atoms[1]->num_atoms;
            else                        n = 1;
        else                            n = 0;

        if (vtableType == NULL)
            vtableType = PointerType::getUnqual(StructType::create(C, "VTable"));

        static FunctionType *LikelyDispatchSignature = NULL;
        if (LikelyDispatchSignature == NULL) {
            vector<Type*> dispatchParameters;
            dispatchParameters.push_back(vtableType);
            dispatchParameters.push_back(PointerType::getUnqual(Matrix));
            LikelyDispatchSignature = FunctionType::get(Matrix, dispatchParameters, false);
        }
        likelyDispatch = Function::Create(LikelyDispatchSignature, GlobalValue::ExternalLinkage, "likely_dispatch", module);
        likelyDispatch->setCallingConv(CallingConv::C);
        likelyDispatch->setDoesNotAlias(0);
        likelyDispatch->setDoesNotAlias(1);
        likelyDispatch->setDoesNotAlias(2);
        likelyDispatch->setDoesNotCapture(1);
        likelyDispatch->setDoesNotCapture(2);
    }

    ~VTable()
    {
        likely_release_ast(ast);
        likely_release_env(env);
        for (FunctionBuilder *function : functions)
            delete function;
    }

    likely_function compile()
    {
        static FunctionType* functionType = NULL;
        if (functionType == NULL)
            functionType = FunctionType::get(Matrix, Matrix, true);
        Function *function = getFunction(functionType);
        BasicBlock *entry = BasicBlock::Create(C, "entry", function);
        IRBuilder<> builder(entry);

        Value *array;
        if (n > 0) {
            array = builder.CreateAlloca(Matrix, Constant::getIntegerValue(Type::getInt32Ty(C), APInt(32, (uint64_t)n)));
            builder.CreateStore(function->arg_begin(), builder.CreateGEP(array, Constant::getIntegerValue(NativeIntegerType, APInt(8*sizeof(void*), 0))));
            if (n > 1) {
                Value *vaList = builder.CreateAlloca(IntegerType::getInt8PtrTy(C));
                Value *vaListRef = builder.CreateBitCast(vaList, Type::getInt8PtrTy(C));
                builder.CreateCall(Intrinsic::getDeclaration(module, Intrinsic::vastart), vaListRef);
                for (likely_arity i=1; i<n; i++)
                    builder.CreateStore(builder.CreateVAArg(vaList, Matrix), builder.CreateGEP(array, Constant::getIntegerValue(NativeIntegerType, APInt(8*sizeof(void*), i))));
                builder.CreateCall(Intrinsic::getDeclaration(module, Intrinsic::vaend), vaListRef);
            }
        } else {
            array = ConstantPointerNull::get(PointerType::getUnqual(Matrix));
        }
        builder.CreateRet(builder.CreateCall2(likelyDispatch, thisVTable(), array));
        return reinterpret_cast<likely_function>(finalize(function));
    }

    likely_function_n compileN()
    {
        static FunctionType* functionType = NULL;
        if (functionType == NULL)
            functionType = FunctionType::get(Matrix, PointerType::getUnqual(Matrix), true);
        Function *function = getFunction(functionType);
        BasicBlock *entry = BasicBlock::Create(C, "entry", function);
        IRBuilder<> builder(entry);
        builder.CreateRet(builder.CreateCall2(likelyDispatch, thisVTable(), function->arg_begin()));
        return reinterpret_cast<likely_function_n>(finalize(function));
    }

private:
    Function *getFunction(FunctionType *functionType) const
    {
        Function *function = cast<Function>(module->getOrInsertFunction(name, functionType));
        function->addFnAttr(Attribute::NoUnwind);
        function->setCallingConv(CallingConv::C);
        function->setDoesNotAlias(0);
        function->setDoesNotAlias(1);
        function->setDoesNotCapture(1);
        return function;
    }

    Constant *thisVTable() const
    {
        return ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(C, 8*sizeof(this)), uintptr_t(this)), vtableType);
    }
};
PointerType *VTable::vtableType = NULL;

} // namespace (anonymous)

LIKELY_EXPORT likely_env likely_new_env()
{
    return new likely_env_struct();
}

LIKELY_EXPORT likely_env likely_retain_env(likely_env env)
{
    if (env) env->ref_count++;
    return env;
}

LIKELY_EXPORT void likely_release_env(likely_env env)
{
    if (!env || --env->ref_count) return;
    delete env;
}

extern "C" LIKELY_EXPORT likely_matrix likely_dispatch(struct VTable *vtable, likely_matrix *m)
{
    void *function = NULL;
    for (size_t i=0; i<vtable->functions.size(); i++) {
        const FunctionBuilder *functionBuilder = vtable->functions[i];
        for (likely_arity j=0; j<vtable->n; j++)
            if (m[j]->type != functionBuilder->type[j])
                goto Next;
        function = functionBuilder->function;
        if (function == NULL)
            return NULL;
        break;
    Next:
        continue;
    }

    if (function == NULL) {
        vector<likely_type> types;
        for (int i=0; i<vtable->n; i++)
            types.push_back(m[i]->type);
        FunctionBuilder *functionBuilder = new FunctionBuilder(vtable->ast, vtable->env, types, true);
        vtable->functions.push_back(functionBuilder);
        function = vtable->functions.back()->function;

        // An impossible case used to ensure that `likely_dispatch` isn't stripped when optimizing executable size
        if (function == NULL)
            likely_dispatch(NULL, NULL);
    }

    typedef likely_matrix (*f0)(void);
    typedef likely_matrix (*f1)(const likely_matrix);
    typedef likely_matrix (*f2)(const likely_matrix, const likely_matrix);
    typedef likely_matrix (*f3)(const likely_matrix, const likely_matrix, const likely_matrix);

    likely_matrix dst;
    switch (vtable->n) {
      case 0: dst = reinterpret_cast<f0>(function)(); break;
      case 1: dst = reinterpret_cast<f1>(function)(m[0]); break;
      case 2: dst = reinterpret_cast<f2>(function)(m[0], m[1]); break;
      case 3: dst = reinterpret_cast<f3>(function)(m[0], m[1], m[2]); break;
      default: dst = NULL; likely_assert(false, "likely_dispatch invalid arity: %d", vtable->n);
    }

    return dst;
}

likely_function likely_compile(likely_ast ast, likely_env env)
{
    if (!ast || !env) return NULL;
    return (new VTable(ast, env))->compile();
}

likely_function_n likely_compile_n(likely_ast ast, likely_env env)
{
    if (!ast || !env) return NULL;
    return (new VTable(ast, env))->compileN();
}

void likely_compile_to_file(likely_ast ast, likely_env env, const char *symbol_name, likely_type *types, likely_arity n, const char *file_name, bool native)
{
    if (!ast || !env) return;
    FunctionBuilder(ast, env, vector<likely_type>(types, types+n), native, symbol_name).write(file_name);
}

likely_matrix likely_eval(likely_ast ast, likely_env env)
{
    if (!ast || !env) return NULL;
    likely_ast expr = likely_ast_from_string("(lambda () (scalar <ast>))");
    expr->atoms[2]->atoms[1] = likely_retain_ast(ast);
    FunctionBuilder functionBuilder(expr, env, vector<likely_type>(), true);
    likely_release_ast(expr);
    if (functionBuilder.function) return reinterpret_cast<likely_matrix(*)(void)>(functionBuilder.function)();
    else                          return NULL;
}
