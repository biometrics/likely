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

using namespace llvm;
using namespace std;

#define LLVM_VALUE_IS_INT(VALUE) (llvm::isa<Constant>(VALUE))
#define LLVM_VALUE_TO_INT(VALUE) (llvm::cast<Constant>(VALUE)->getUniqueInteger().getZExtValue())

static likely_type likely_type_native = likely_type_null;
static IntegerType *NativeIntegerType = NULL;
static PointerType *Matrix = NULL;
static LLVMContext &C = getGlobalContext();

namespace {

struct TypedValue
{
    Value *value;
    likely_type type;
    TypedValue() : value(NULL), type(likely_type_null) {}
    TypedValue(Value *value_, likely_type type_) : value(value_), type(type_) {}
    operator Value*() const { return value; }
    operator likely_type() const { return type; }
    bool isNull() const { return value == NULL; }

    void print(raw_ostream &o) const
    {
        value->print(o);
        o << "; " << likely_type_to_string(type) << "\n";
    }

    void dump() const { print(dbgs()); }
};

struct ExpressionBuilder : public IRBuilder<>
{
    Module *module;
    string name;
    vector<likely_type> types;
    TargetMachine *targetMachine;

    typedef map<string,TypedValue> Closure;
    vector<Closure> closures;

    ExpressionBuilder(Module *module, const string &name, const vector<likely_type> &types, TargetMachine *targetMachine = NULL)
        : IRBuilder<>(C), module(module), name(name), types(types), targetMachine(targetMachine)
    {}

    static TypedValue constant(double value, likely_type type = likely_type_native)
    {
        const int depth = likely_depth(type);
        if (likely_floating(type)) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return TypedValue(ConstantFP::get(Type::getDoubleTy(C), value), type);
            else if (depth == 32) return TypedValue(ConstantFP::get(Type::getFloatTy(C), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return TypedValue(); }
        } else {
            return TypedValue(Constant::getIntegerValue(Type::getIntNTy(C, depth), APInt(depth, uint64_t(value))), type);
        }
    }

    static TypedValue zero(likely_type type = likely_type_native) { return constant(0, type); }
    static TypedValue one (likely_type type = likely_type_native) { return constant(1, type); }
    static TypedValue intMax(likely_type type) { const int bits = likely_depth(type); return constant((1 << (bits - (likely_signed(type) ? 1 : 0)))-1, bits); }
    static TypedValue intMin(likely_type type) { const int bits = likely_depth(type); return constant(likely_signed(type) ? (1 << (bits - 1)) : 0, bits); }
    static TypedValue type(likely_type type) { return constant(type, int(sizeof(likely_type)*8)); }

    TypedValue data    (const TypedValue &matrix) { return TypedValue(CreatePointerCast(CreateLoad(CreateStructGEP(matrix, 1), "data"), ty(matrix, true)), matrix.type & likely_type_mask); }
    TypedValue channels(const TypedValue &matrix) { return likely_multi_channel(matrix) ? TypedValue(CreateLoad(CreateStructGEP(matrix, 2), "channels"), likely_type_native) : one(); }
    TypedValue columns (const TypedValue &matrix) { return likely_multi_column (matrix) ? TypedValue(CreateLoad(CreateStructGEP(matrix, 3), "columns" ), likely_type_native) : one(); }
    TypedValue rows    (const TypedValue &matrix) { return likely_multi_row    (matrix) ? TypedValue(CreateLoad(CreateStructGEP(matrix, 4), "rows"    ), likely_type_native) : one(); }
    TypedValue frames  (const TypedValue &matrix) { return likely_multi_frame  (matrix) ? TypedValue(CreateLoad(CreateStructGEP(matrix, 5), "frames"  ), likely_type_native) : one(); }

    void steps(const TypedValue &matrix, Value **columnStep, Value **rowStep, Value **frameStep)
    {
        *columnStep = channels(matrix);
        *rowStep    = CreateMul(columns(matrix), *columnStep, "y_step");
        *frameStep  = CreateMul(rows(matrix), *rowStep, "t_step");
    }

    TypedValue cast(const TypedValue &x, likely_type type)
    {
        if ((x.type & likely_type_mask) == (type & likely_type_mask))
            return x;
        Type *dstType = ty(type);
        return TypedValue(CreateCast(CastInst::getCastOpcode(x, likely_signed(x.type), dstType, likely_signed(type)), x, dstType), type);
    }

    void addVariable(const string &name, const TypedValue &value)
    {
        Closure &closure = closures.back();
        Closure::iterator it = closure.find(name);
        if (it != closure.end()) {
            // Update variable
            CreateStore(value, it->second);
        } else {
            // New variable
            AllocaInst *variable = CreateAlloca(ty(value), 0, name);
            CreateStore(value, variable);
            closure.insert(pair<string,TypedValue>(name, TypedValue(variable, value)));
        }
    }

    TypedValue getVariable(const string &name)
    {
        for (vector<Closure>::reverse_iterator rit = closures.rbegin(); rit != closures.rend(); rit++) {
            Closure::iterator it = rit->find(name);
            if (it != rit->end()) {
                LoadInst *variable = CreateLoad(it->second, name);
                return TypedValue(variable, it->second);
            }
        }
        return TypedValue();
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

    Function *getKernel(Type *ret, Type *dst = NULL, Type *start = NULL, Type *stop = NULL)
    {
        Function *kernel;
        const string name = this->name + (dst == NULL ? "" : "_thunk");
        switch (types.size()) {
          case 0: kernel = ::cast<Function>(module->getOrInsertFunction(name, ret, dst, start, stop, NULL)); break;
          case 1: kernel = ::cast<Function>(module->getOrInsertFunction(name, ret, Matrix, dst, start, stop, NULL)); break;
          case 2: kernel = ::cast<Function>(module->getOrInsertFunction(name, ret, Matrix, Matrix, dst, start, stop, NULL)); break;
          case 3: kernel = ::cast<Function>(module->getOrInsertFunction(name, ret, Matrix, Matrix, Matrix, dst, start, stop, NULL)); break;
          default: { kernel = NULL; likely_assert(false, "FunctionBuilder::getFunction invalid arity: %zu", types.size()); }
        }
        kernel->addFnAttr(Attribute::NoUnwind);
        kernel->setCallingConv(CallingConv::C);
        if (ret->isPointerTy())
            kernel->setDoesNotAlias(0);
        size_t argc = types.size();
        if (dst) argc++;
        for (size_t i=0; i<argc; i++) {
            kernel->setDoesNotAlias((unsigned int) i+1);
            kernel->setDoesNotCapture((unsigned int) i+1);
        }
        return kernel;
    }

    vector<TypedValue> getArgs(Function *function)
    {
        vector<TypedValue> result;
        Function::arg_iterator args = function->arg_begin();
        likely_arity n = 0;
        while (args != function->arg_end()) {
            Value *src = args++;
            stringstream name; name << "arg_" << int(n);
            src->setName(name.str());
            result.push_back(TypedValue(src, n < types.size() ? types[n] : likely_type_null));
            n++;
        }
        return result;
    }
};

struct Operation
{
    static map<string, stack<shared_ptr<Operation>>> operations; // stack allows shadowing

    static TypedValue expression(ExpressionBuilder &builder, likely_ast ast)
    {
        string operator_;
        if (ast->is_list) {
            likely_assert((ast->num_atoms > 0) && !ast->atoms[0]->is_list, "ill-formed abstract syntax tree");
            operator_ = ast->atoms[0]->atom;
        } else {
            operator_ = ast->atom;
        }

        map<string,stack<shared_ptr<Operation>>>::iterator it = operations.find(operator_);
        if (it != operations.end())
            return it->second.top()->call(builder, ast);

        const TypedValue variable = builder.getVariable(operator_);
        if (!variable.isNull())
            return variable;

        if ((operator_.front() == '"') && (operator_.back() == '"'))
            return TypedValue(builder.CreateGlobalStringPtr(operator_.substr(1, operator_.length()-2)), likely_type_null);

        bool ok;
        TypedValue c = constant(operator_, &ok);
        if (ok) return c;

        likely_type type = likely_type_from_string(operator_.c_str());
        if (type != likely_type_null)
            return ExpressionBuilder::constant(type, likely_type_u32);

        likely_throw(ast->is_list ? ast->atoms[0] : ast, "unrecognized literal");
        return TypedValue();
    }

    virtual ~Operation() {}

protected:
    static TypedValue constant(const string &str, bool *ok)
    {
        char *p;
        const double value = strtod(str.c_str(), &p);
        const likely_type type = likely_type_from_value(value);
        *ok = (*p == 0);
        return ExpressionBuilder::constant(value, type);
    }

    static likely_type validFloatType(likely_type type)
    {
        likely_set_floating(&type, true);
        likely_set_signed(&type, true);
        likely_set_depth(&type, likely_depth(type) > 32 ? 64 : 32);
        return type;
    }

    virtual TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
    {
        vector<TypedValue> operands;
        if (ast->is_list)
            for (size_t i=1; i<ast->num_atoms; i++)
                operands.push_back(expression(builder, ast->atoms[i]));
        return call(builder, operands);
    }

    virtual TypedValue call(ExpressionBuilder &builder, const vector<TypedValue> &args) const = 0;
};
map<string, stack<shared_ptr<Operation>>> Operation::operations;

template <class T>
struct RegisterOperation
{
    RegisterOperation(const string &symbol)
    {
        Operation::operations[symbol].push(shared_ptr<Operation>(new T()));
    }
};
#define LIKELY_REGISTER_OPERATION(OP, SYM) static struct RegisterOperation<OP##Operation> Register##OP##Operation(SYM);
#define LIKELY_REGISTER(OP) LIKELY_REGISTER_OPERATION(OP, #OP)

class NullaryOperation : public Operation
{
    TypedValue call(ExpressionBuilder &builder, const vector<TypedValue> &args) const
    {
        assert(args.empty());
        (void) args;
        return callNullary(builder);
    }
    virtual TypedValue callNullary(ExpressionBuilder &builder) const = 0;
};

#define LIKELY_REGISTER_TYPE(OP)                                    \
class OP##Operation : public NullaryOperation                       \
{                                                                   \
    TypedValue callNullary(ExpressionBuilder &builder) const        \
    {                                                               \
        return builder.constant(likely_type_##OP, likely_type_u32); \
    }                                                               \
};                                                                  \
LIKELY_REGISTER(OP)                                                 \

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

class UnaryOperation : public Operation
{
    TypedValue call(ExpressionBuilder &builder, const vector<TypedValue> &args) const
    {
        assert(args.size() == 1);
        return callUnary(builder, args[0]);
    }
    virtual TypedValue callUnary(ExpressionBuilder &builder, const TypedValue &arg) const = 0;
};

class notOperation : public UnaryOperation
{
    TypedValue callUnary(ExpressionBuilder &builder, const TypedValue &arg) const
    {
        likely_type type = arg.type;
        likely_set_signed(&type, false);
        likely_set_floating(&type, false);
        return TypedValue(builder.CreateXor(builder.intMax(type), arg.value), type);
    }
};
LIKELY_REGISTER_OPERATION(not, "~")

class typeOperation : public UnaryOperation
{
    TypedValue callUnary(ExpressionBuilder &builder, const TypedValue &arg) const
    {
        (void) builder;
        return ExpressionBuilder::type(arg);
    }
};
LIKELY_REGISTER(type)

class scalarOperation : public UnaryOperation
{
    TypedValue callUnary(ExpressionBuilder &builder, const TypedValue &arg) const
    {
        if (arg.isNull())
            return builder.zero();

        if (arg.value->getType() == Matrix)
            return arg;

        static FunctionType* LikelyScalarSignature = NULL;
        if (LikelyScalarSignature == NULL)
            LikelyScalarSignature = FunctionType::get(Matrix, Type::getDoubleTy(C), false);

        Function *likelyScalar = Function::Create(LikelyScalarSignature, GlobalValue::ExternalLinkage, "likely_scalar", builder.module);
        likelyScalar->setCallingConv(CallingConv::C);
        likelyScalar->setDoesNotAlias(0);

         // An impossible case used to ensure that `likely_scalar` isn't stripped when optimizing executable size
        if (likelyScalar == NULL)
            likely_scalar(0);

        return TypedValue(builder.CreateCall(likelyScalar, builder.cast(arg, likely_type_f64)), arg.type);
    }
};
LIKELY_REGISTER(scalar)

class UnaryMathOperation : public UnaryOperation
{
    TypedValue callUnary(ExpressionBuilder &builder, const TypedValue &x) const
    {
        TypedValue xc = builder.cast(x, validFloatType(x.type));
        vector<Type*> args;
        args.push_back(xc.value->getType());
        return TypedValue(builder.CreateCall(Intrinsic::getDeclaration(builder.module, id(), args), xc), xc.type);
    }
    virtual Intrinsic::ID id() const = 0;
};

#define LIKELY_REGISTER_UNARY_MATH(OP)                 \
class OP##Operation : public UnaryMathOperation        \
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

class BinaryOperation : public Operation
{
    TypedValue call(ExpressionBuilder &builder, const vector<TypedValue> &args) const
    {
        assert(args.size() == 2);
        return callBinary(builder, args[0], args[1]);
    }
    virtual TypedValue callBinary(ExpressionBuilder &builder, const TypedValue &arg1, const TypedValue &arg2) const = 0;
};

class castOperation : public BinaryOperation
{
    TypedValue callBinary(ExpressionBuilder &builder, const TypedValue &x, const TypedValue &type) const
    {
        return builder.cast(x, (likely_type)LLVM_VALUE_TO_INT(type.value));
    }
};
LIKELY_REGISTER(cast)

class ArithmeticOperation : public BinaryOperation
{
    TypedValue callBinary(ExpressionBuilder &builder, const TypedValue &arg1, const TypedValue &arg2) const
    {
        likely_type type = likely_type_from_types(arg1, arg2);
        return callArithmetic(builder, builder.cast(arg1, type), builder.cast(arg2, type), type);
    }
    virtual TypedValue callArithmetic(ExpressionBuilder &builder, const TypedValue &lhs, const TypedValue &rhs, likely_type type) const = 0;
};

class addOperation : public ArithmeticOperation
{
    TypedValue callArithmetic(ExpressionBuilder &builder, const TypedValue &lhs, const TypedValue &rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(builder.CreateFAdd(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module, likely_signed(type) ? Intrinsic::sadd_with_overflow : Intrinsic::uadd_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = likely_signed(type) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, ExpressionBuilder::zero(type)), builder.intMax(type), builder.intMin(type)) : ExpressionBuilder::intMax(type);
                return TypedValue(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), type);
            } else {
                return TypedValue(builder.CreateAdd(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_OPERATION(add, "+")

class subtractOperation : public ArithmeticOperation
{
    TypedValue callArithmetic(ExpressionBuilder &builder, const TypedValue &lhs, const TypedValue &rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(builder.CreateFSub(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module, likely_signed(type) ? Intrinsic::ssub_with_overflow : Intrinsic::usub_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *overflowResult = likely_signed(type) ? builder.CreateSelect(builder.CreateICmpSGE(lhs, ExpressionBuilder::zero(type)), builder.intMax(type), builder.intMin(type)) : ExpressionBuilder::intMin(type);
                return TypedValue(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), type);
            } else {
                return TypedValue(builder.CreateSub(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_OPERATION(subtract, "-")

class multiplyOperation : public ArithmeticOperation
{
    TypedValue callArithmetic(ExpressionBuilder &builder, const TypedValue &lhs, const TypedValue &rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(builder.CreateFMul(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                CallInst *result = builder.CreateCall2(Intrinsic::getDeclaration(builder.module, likely_signed(type) ? Intrinsic::smul_with_overflow : Intrinsic::umul_with_overflow, lhs.value->getType()), lhs, rhs);
                Value *zero = ExpressionBuilder::zero(type);
                Value *overflowResult = likely_signed(type) ? builder.CreateSelect(builder.CreateXor(builder.CreateICmpSGE(lhs, zero), builder.CreateICmpSGE(rhs, zero)), builder.intMin(type), builder.intMax(type)) : ExpressionBuilder::intMax(type);
                return TypedValue(builder.CreateSelect(builder.CreateExtractValue(result, 1), overflowResult, builder.CreateExtractValue(result, 0)), type);
            } else {
                return TypedValue(builder.CreateMul(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_OPERATION(multiply, "*")

class divideOperation : public ArithmeticOperation
{
    TypedValue callArithmetic(ExpressionBuilder &builder, const TypedValue &n, const TypedValue &d, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(builder.CreateFDiv(n, d), type);
        } else {
            if (likely_signed(type)) {
                if (likely_saturation(type)) {
                    Value *safe_i = builder.CreateAdd(n, builder.CreateZExt(builder.CreateICmpNE(builder.CreateOr(builder.CreateAdd(d, ExpressionBuilder::one(type)), builder.CreateAdd(n, ExpressionBuilder::intMin(type))), ExpressionBuilder::zero(type)), n.value->getType()));
                    return TypedValue(builder.CreateSDiv(safe_i, d), type);
                } else {
                    return TypedValue(builder.CreateSDiv(n, d), type);
                }
            } else {
                return TypedValue(builder.CreateUDiv(n, d), type);
            }
        }
    }
};
LIKELY_REGISTER_OPERATION(divide, "/")

class remOperation : public ArithmeticOperation
{
    TypedValue callArithmetic(ExpressionBuilder &builder, const TypedValue &lhs, const TypedValue &rhs, likely_type type) const
    {
        return TypedValue(likely_floating(type) ? builder.CreateFRem(lhs, rhs) : (likely_signed(type) ? builder.CreateSRem(lhs, rhs) : builder.CreateURem(lhs, rhs)), type);
    }
};
LIKELY_REGISTER_OPERATION(rem, "%")

#define LIKELY_REGISTER_LOGIC(OP, SYM)                                                                                          \
class OP##Operation : public ArithmeticOperation                                                                                \
{                                                                                                                               \
    TypedValue callArithmetic(ExpressionBuilder &builder, const TypedValue &lhs, const TypedValue &rhs, likely_type type) const \
    {                                                                                                                           \
        return TypedValue(builder.Create##OP(lhs.value, rhs.value), type);                                                      \
    }                                                                                                                           \
};                                                                                                                              \
LIKELY_REGISTER_OPERATION(OP, SYM)                                                                                              \

LIKELY_REGISTER_LOGIC(And, "and")
LIKELY_REGISTER_LOGIC(Or, "or")
LIKELY_REGISTER_LOGIC(Xor, "xor")
LIKELY_REGISTER_LOGIC(Shl, "shl")
LIKELY_REGISTER_LOGIC(LShr, "lshr")
LIKELY_REGISTER_LOGIC(AShr, "ashr")

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                                     \
class OP##Operation : public ArithmeticOperation                                                                                \
{                                                                                                                               \
    TypedValue callArithmetic(ExpressionBuilder &builder, const TypedValue &lhs, const TypedValue &rhs, likely_type type) const \
    {                                                                                                                           \
        return TypedValue(likely_floating(type) ? builder.CreateFCmpO##OP(lhs, rhs)                                             \
                                                : (likely_signed(type) ? builder.CreateICmpS##OP(lhs, rhs)                      \
                                                                       : builder.CreateICmpU##OP(lhs, rhs)), type);             \
    }                                                                                                                           \
};                                                                                                                              \
LIKELY_REGISTER_OPERATION(OP, SYM)                                                                                              \

LIKELY_REGISTER_COMPARISON(LT, "<")
LIKELY_REGISTER_COMPARISON(LE, "<=")
LIKELY_REGISTER_COMPARISON(GT, ">")
LIKELY_REGISTER_COMPARISON(GE, ">=")

#define LIKELY_REGISTER_EQUALITY(OP, SYM)                                                                                       \
class OP##Operation : public ArithmeticOperation                                                                                \
{                                                                                                                               \
    TypedValue callArithmetic(ExpressionBuilder &builder, const TypedValue &lhs, const TypedValue &rhs, likely_type type) const \
    {                                                                                                                           \
        return TypedValue(likely_floating(type) ? builder.CreateFCmpO##OP(lhs, rhs)                                             \
                                                : builder.CreateICmp##OP(lhs, rhs), type);                                      \
    }                                                                                                                           \
};                                                                                                                              \
LIKELY_REGISTER_OPERATION(OP, SYM)                                                                                              \

LIKELY_REGISTER_EQUALITY(EQ, "==")
LIKELY_REGISTER_EQUALITY(NE, "!=")

class BinaryMathOperation : public BinaryOperation
{
    TypedValue callBinary(ExpressionBuilder &builder, const TypedValue &x, const TypedValue &n) const
    {
        const likely_type type = nIsInteger() ? x.type : likely_type_from_types(x, n);
        TypedValue xc = builder.cast(x, validFloatType(type));
        TypedValue nc = builder.cast(n, nIsInteger() ? likely_type_i32 : xc.type);
        vector<Type*> args;
        args.push_back(xc.value->getType());
        return TypedValue(builder.CreateCall2(Intrinsic::getDeclaration(builder.module, id(), args), xc, nc), xc.type);
    }
    virtual Intrinsic::ID id() const = 0;
    virtual bool nIsInteger() const { return false; }
};

class powiOperation : public BinaryMathOperation
{
    Intrinsic::ID id() const { return Intrinsic::powi; }
    bool nIsInteger() const { return true; }
};
LIKELY_REGISTER(powi)

#define LIKELY_REGISTER_BINARY_MATH(OP)                \
class OP##Operation : public BinaryMathOperation       \
{                                                      \
    Intrinsic::ID id() const { return Intrinsic::OP; } \
};                                                     \
LIKELY_REGISTER(OP)                                    \

LIKELY_REGISTER_BINARY_MATH(pow)
LIKELY_REGISTER_BINARY_MATH(copysign)

class TernaryOperation : public Operation
{
    TypedValue call(ExpressionBuilder &builder, const vector<TypedValue> &args) const
    {
        assert(args.size() == 3);
        return callTernary(builder, args[0], args[1], args[2]);
    }
    virtual TypedValue callTernary(ExpressionBuilder &builder, const TypedValue &arg1, const TypedValue &arg2, const TypedValue &arg3) const = 0;
};

class fmaOperation : public TernaryOperation
{
    TypedValue callTernary(ExpressionBuilder &builder, const TypedValue &a, const TypedValue &b, const TypedValue &c) const
    {
        const likely_type type = likely_type_from_types(likely_type_from_types(a, b), c);
        TypedValue ac = builder.cast(a, validFloatType(type));
        TypedValue bc = builder.cast(b, ac.type);
        TypedValue cc = builder.cast(c, ac.type);
        vector<Type*> args;
        args.push_back(ac.value->getType());
        return TypedValue(builder.CreateCall3(Intrinsic::getDeclaration(builder.module, Intrinsic::fma, args), ac, bc, cc), ac.type);
    }
};
LIKELY_REGISTER(fma)

class selectOperation : public TernaryOperation
{
    TypedValue callTernary(ExpressionBuilder &builder, const TypedValue &c, const TypedValue &t, const TypedValue &f) const
    {
         const likely_type type = likely_type_from_types(t, f);
         return TypedValue(builder.CreateSelect(c,  builder.cast(t, type), builder.cast(f, type)), type);
    }
};
LIKELY_REGISTER(select)

class GenericOperation : public Operation
{
    TypedValue call(ExpressionBuilder &builder, const vector<TypedValue> &args) const
    {
        (void) builder;
        (void) args;
        likely_assert(false, "generic operation logic error");
        return TypedValue();
    }

protected:
    void addToClosure(ExpressionBuilder &builder, likely_ast ast) const
    {
        likely_assert(ast->is_list, "invalid closure");
        if ((ast->num_atoms == 2) && !ast->atoms[0]->is_list) {
            builder.addVariable(ast->atoms[0]->atom, expression(builder, ast->atoms[1]));
        } else {
            for (size_t i=0; i<ast->num_atoms; i++)
                addToClosure(builder, ast->atoms[i]);
        }
    }
};

class defineOperation : public GenericOperation
{
    class Definition : public NullaryOperation
    {
        likely_ast ast;

    public:
        Definition(likely_ast ast)
            : ast(ast)
        {
            likely_retain_ast(ast);
        }

        ~Definition()
        {
            likely_release_ast(ast);
        }

    private:
        TypedValue callNullary(ExpressionBuilder &builder) const
        {
            return expression(builder, ast);
        }
    };

    using Operation::call;
    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
    {
        (void) builder;
        operations[ast->atoms[1]->atom].push(shared_ptr<Operation>(new Definition(ast->atoms[2])));
        return TypedValue();
    }
};
LIKELY_REGISTER(define)

class lambdaOperation : public GenericOperation
{
    using Operation::call;
    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
    {
        (void) builder;
        (void) ast;
        return TypedValue();
    }
};
LIKELY_REGISTER(lambda)

class kernelOperation : public GenericOperation
{
    class kernelArgOperation : public NullaryOperation
    {
        TypedValue matrix;
        likely_type kernel;
        MDNode *node;

    public:
        kernelArgOperation(const TypedValue &matrix, likely_type kernel, MDNode *node)
            : matrix(matrix), kernel(kernel), node(node) {}

    private:
        TypedValue callNullary(ExpressionBuilder &builder) const
        {
            Value *i;
            if (((matrix ^ kernel) & likely_type_multi_dimension) == 0) {
                // This matrix has the same dimensionality as the kernel
                i = builder.getVariable("i");
            } else {
                Value *columnStep, *rowStep, *frameStep;
                builder.steps(matrix, &columnStep, &rowStep, &frameStep);
                i = ExpressionBuilder::zero();
                if (likely_multi_channel(matrix)) i = builder.getVariable("c");
                if (likely_multi_column(matrix))  i = builder.CreateAdd(builder.CreateMul(builder.getVariable("x"), columnStep), i);
                if (likely_multi_row(matrix))     i = builder.CreateAdd(builder.CreateMul(builder.getVariable("y"), rowStep), i);
                if (likely_multi_frame(matrix))   i = builder.CreateAdd(builder.CreateMul(builder.getVariable("t"), frameStep), i);
            }

            LoadInst *load = builder.CreateLoad(builder.CreateGEP(builder.data(matrix), i));
            load->setMetadata("llvm.mem.parallel_loop_access", node);
            return TypedValue(load, matrix.type);
        }
    };

    using Operation::call;
    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
    {
        Function *function = builder.getKernel(Matrix);
        vector<TypedValue> srcs = builder.getArgs(function);
        BasicBlock *entry = BasicBlock::Create(C, "entry", function);
        builder.SetInsertPoint(entry);

        TypedValue dstChannels = getDimensions(builder, ast, "channels", srcs);
        TypedValue dstColumns  = getDimensions(builder, ast, "columns", srcs);
        TypedValue dstRows     = getDimensions(builder, ast, "rows", srcs);
        TypedValue dstFrames   = getDimensions(builder, ast, "frames", srcs);

        Function *thunk;
        likely_type dstType;
        {
            thunk = builder.getKernel(Type::getVoidTy(C), Matrix, NativeIntegerType, NativeIntegerType);
            BasicBlock *entry = BasicBlock::Create(C, "entry", thunk);
            builder.SetInsertPoint(entry);
            vector<TypedValue> srcs = builder.getArgs(thunk);
            TypedValue stop = srcs.back(); srcs.pop_back();
            stop.value->setName("stop");
            stop.type = likely_type_native;
            TypedValue start = srcs.back(); srcs.pop_back();
            start.value->setName("start");
            start.type = likely_type_native;
            TypedValue dst = srcs.back(); srcs.pop_back();
            dst.value->setName("dst");

            likely_set_multi_channel(&dst.type, likely_multi_channel(dstChannels.type));
            likely_set_multi_column (&dst.type, likely_multi_column (dstColumns.type));
            likely_set_multi_row    (&dst.type, likely_multi_row    (dstRows.type));
            likely_set_multi_frame  (&dst.type, likely_multi_frame  (dstFrames.type));

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
            builder.steps(dst, &columnStep, &rowStep, &frameStep);
            Value *frameRemainder = builder.CreateURem(i, frameStep, "t_rem");
            TypedValue t(builder.CreateUDiv(i, frameStep, "t"), likely_type_native);
            Value *rowRemainder = builder.CreateURem(frameRemainder, rowStep, "y_rem");
            TypedValue y(builder.CreateUDiv(frameRemainder, rowStep, "y"), likely_type_native);
            Value *columnRemainder = builder.CreateURem(rowRemainder, columnStep, "c");
            TypedValue x(builder.CreateUDiv(rowRemainder, columnStep, "x"), likely_type_native);
            TypedValue c(columnRemainder, likely_type_native);

            builder.closures.push_back(ExpressionBuilder::Closure());
            builder.addVariable("i", TypedValue(i, likely_type_native));
            builder.addVariable("c", c);
            builder.addVariable("x", x);
            builder.addVariable("y", y);
            builder.addVariable("t", t);

            const likely_ast args = ast->atoms[1];
            assert(args->num_atoms == srcs.size());
            for (size_t j=0; j<args->num_atoms; j++)
                Operation::operations[args->atoms[j]->atom].push(shared_ptr<Operation>(new kernelArgOperation(srcs[j], dst.type, node)));

            TypedValue result = Operation::expression(builder, ast->atoms[2]);
            dstType = dst.type = result.type;
            StoreInst *store = builder.CreateStore(result, builder.CreateGEP(builder.data(dst), i));
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
                Operation::operations[args->atoms[i]->atom].pop();
            builder.closures.pop_back();
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
        likelyNewArgs.push_back(ExpressionBuilder::type(dstType));
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

        if (!builder.types.empty() && likely_parallel(builder.types[0])) {
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
            likelyForkArgs.push_back(ExpressionBuilder::constant((double)builder.types.size(), 8));
            likelyForkArgs.push_back(kernelSize);
            likelyForkArgs.insert(likelyForkArgs.end(), srcs.begin(), srcs.end());
            likelyForkArgs.push_back(dst);
            builder.CreateCall(likelyFork, likelyForkArgs);
        } else {
            vector<Value*> thunkArgs;
            for (const TypedValue &src : srcs)
                thunkArgs.push_back(src.value);
            thunkArgs.push_back(dst);
            thunkArgs.push_back(ExpressionBuilder::zero());
            thunkArgs.push_back(kernelSize);
            builder.CreateCall(thunk, thunkArgs);
        }

        builder.CreateRet(dst);
        return TypedValue(function, dstType);
    }

    static TypedValue getDimensions(ExpressionBuilder &builder, likely_ast ast, const char *axis, vector<TypedValue> &srcs)
    {
        Value *result = NULL;

        // Look for a dimensionality expression
        for (size_t i=3; i<ast->num_atoms; i++) {
            if (ast->atoms[i]->is_list && (ast->atoms[i]->num_atoms == 2) && (!ast->atoms[i]->atoms[0]->is_list) && !strcmp(axis, ast->atoms[i]->atoms[0]->atom)) {
                result = builder.cast(Operation::expression(builder, ast->atoms[i]->atoms[1]), likely_type_native);
                break;
            }
        }

        // Use default dimensionality
        if (result == NULL) {
            if (srcs.empty()) {
                result = ExpressionBuilder::constant(1);
            } else {
                if      (!strcmp(axis, "channels")) result = builder.channels(srcs[0]);
                else if (!strcmp(axis, "columns"))  result = builder.columns (srcs[0]);
                else if (!strcmp(axis, "rows"))     result = builder.rows    (srcs[0]);
                else                                result = builder.frames  (srcs[0]);
            }
        }

        likely_type type = likely_type_native;
        const bool isMulti = (!LLVM_VALUE_IS_INT(result)) || (LLVM_VALUE_TO_INT(result) > 1);
        if      (!strcmp(axis, "channels")) likely_set_multi_channel(&type, isMulti);
        else if (!strcmp(axis, "columns"))  likely_set_multi_column (&type, isMulti);
        else if (!strcmp(axis, "rows"))     likely_set_multi_row    (&type, isMulti);
        else                                likely_set_multi_frame  (&type, isMulti);

        return TypedValue(result, type);
    }
};
LIKELY_REGISTER(kernel)

class functionOperation : public GenericOperation
{
    using Operation::call;

    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
    {
        vector<Type*> types;
        for (likely_type t : builder.types)
            types.push_back(ExpressionBuilder::ty(t));
        Function *tmpFunction = cast<Function>(builder.module->getOrInsertFunction(builder.name+"_tmp", FunctionType::get(Type::getVoidTy(C), types, false)));
        vector<TypedValue> tmpArgs = builder.getArgs(tmpFunction);
        BasicBlock *entry = BasicBlock::Create(C, "entry", tmpFunction);
        builder.SetInsertPoint(entry);

        assert(tmpArgs.size() == ast->atoms[1]->num_atoms);
        builder.closures.push_back(ExpressionBuilder::Closure());
        for (size_t i=0; i<tmpArgs.size(); i++)
            builder.addVariable(ast->atoms[1]->atoms[i]->atom, tmpArgs[i]);
        TypedValue result = expression(builder, ast->atoms[2]);
        builder.closures.pop_back();
        builder.CreateRet(result);

        Function *function = cast<Function>(builder.module->getOrInsertFunction(builder.name, FunctionType::get(result.value->getType(), types, false)));
        vector<TypedValue> args = builder.getArgs(function);

        ValueToValueMapTy VMap;
        for (size_t i=0; i<args.size(); i++)
            VMap[tmpArgs[i]] = args[i];

        SmallVector<ReturnInst*, 1> returns;
        CloneFunctionInto(function, tmpFunction, VMap, false, returns);
        tmpFunction->eraseFromParent();

        return TypedValue(function, result.type);
    }
};
LIKELY_REGISTER(function)

class letOperation : public GenericOperation
{
    using Operation::call;
    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
    {
        builder.closures.push_back(ExpressionBuilder::Closure());
        addToClosure(builder, ast->atoms[1]);
        TypedValue result = expression(builder, ast->atoms[2]);
        builder.closures.pop_back();
        return result;
    }
};
LIKELY_REGISTER(let)

class loopOperation : public GenericOperation
{
    using Operation::call;
    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
    {
        builder.closures.push_back(ExpressionBuilder::Closure());
        addToClosure(builder, ast->atoms[1]);

        BasicBlock *loopCondition = BasicBlock::Create(C, "loop_condition");
        BasicBlock *loopIteration = BasicBlock::Create(C, "loop_iteration");
        BasicBlock *loopEnd = BasicBlock::Create(C, "loop_end");

        builder.CreateBr(loopCondition);
        builder.SetInsertPoint(loopCondition);
        builder.CreateCondBr(expression(builder, ast->atoms[2]), loopIteration, loopEnd);

        builder.SetInsertPoint(loopIteration);
        addToClosure(builder, ast->atoms[3]);
        builder.CreateBr(loopCondition);

        builder.SetInsertPoint(loopEnd);
        TypedValue result = expression(builder, ast->atoms[4]);

        builder.closures.pop_back();
        return result;
    }
};
LIKELY_REGISTER(loop)

#ifdef LIKELY_IO
#include "likely/likely_io.h"

class readOperation : public GenericOperation
{
    using Operation::call;
    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
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
        TypedValue fileName = expression(builder, ast->atoms[1]);
        return TypedValue(builder.CreateCall(likelyRead, fileName), likely_type_null);
    }
};
LIKELY_REGISTER(read)

class writeOperation : public GenericOperation
{
    using Operation::call;
    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
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
        likelyWriteArguments.push_back(expression(builder, ast->atoms[1]));
        likelyWriteArguments.push_back(expression(builder, ast->atoms[2]));
        return TypedValue(builder.CreateCall(likelyWrite, likelyWriteArguments), likely_type_null);
    }
};
LIKELY_REGISTER(write)

class decodeOperation : public GenericOperation
{
    using Operation::call;
    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
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
        TypedValue matrix = expression(builder, ast->atoms[1]);
        return TypedValue(builder.CreateCall(likelyDecode, matrix), likely_type_null);
    }
};
LIKELY_REGISTER(decode)

class encodeOperation : public GenericOperation
{
    using Operation::call;
    TypedValue call(ExpressionBuilder &builder, likely_ast ast) const
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
        likelyEncodeArguments.push_back(expression(builder, ast->atoms[1]));
        likelyEncodeArguments.push_back(expression(builder, ast->atoms[2]));
        return TypedValue(builder.CreateCall(likelyEncode, likelyEncodeArguments), likely_type_null);
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

            likely_set_depth(&likely_type_native, sizeof(likely_size)*8);
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

    FunctionBuilder(likely_ast ast, const vector<likely_type> &types, bool native, const string &symbol_name = string())
        : JITResources(native, symbol_name)
    {
        type = new likely_type[types.size()];
        memcpy(type, types.data(), sizeof(likely_type) * types.size());

        ExpressionBuilder builder(module, name, types, native ? targetMachine : NULL);
        Function *result = cast<Function>(Operation::expression(builder, ast).value);
        function = finalize(result);
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

    ~FunctionBuilder()
    {
        delete[] type;
    }
};

struct VTable : public JITResources
{
    static PointerType *vtableType;
    likely_ast ast;
    likely_arity n;
    vector<FunctionBuilder*> functions;
    Function *likelyDispatch;

    VTable(likely_ast ast)
        : JITResources(true), ast(ast)
    {
        n = likely_get_arity(ast);

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
        likely_retain_ast(ast);
    }

    ~VTable()
    {
        likely_release_ast(ast);
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

extern "C" LIKELY_EXPORT likely_matrix likely_dispatch(struct VTable *vtable, likely_matrix *m)
{
    void *function = NULL;
    for (size_t i=0; i<vtable->functions.size(); i++) {
        const FunctionBuilder *functionBuilder = vtable->functions[i];
        for (likely_arity j=0; j<vtable->n; j++)
            if (m[j]->type != functionBuilder->type[j])
                goto Next;
        function = functionBuilder->function;
        break;
    Next:
        continue;
    }

    if (function == NULL) {
        vector<likely_type> types;
        for (int i=0; i<vtable->n; i++)
            types.push_back(m[i]->type);
        FunctionBuilder *functionBuilder = new FunctionBuilder(vtable->ast, types, true);
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

likely_function likely_compile(likely_ast ast)
{
    return (new VTable(ast))->compile();
}

likely_function_n likely_compile_n(likely_ast ast)
{
    return (new VTable(ast))->compileN();
}

void likely_compile_to_file(likely_ast ast, const char *symbol_name, likely_type *types, likely_arity n, const char *file_name, bool native)
{
    FunctionBuilder(ast, vector<likely_type>(types, types+n), native, symbol_name).write(file_name);
}

likely_matrix likely_eval(likely_ast ast)
{
    if (ast == NULL)
        return NULL;

    likely_ast expr = likely_ast_from_string("(function () (scalar <ast>))");
    expr->atoms[2]->atoms[1] = likely_retain_ast(ast);
    FunctionBuilder functionBuilder(expr, vector<likely_type>(), true);
    likely_release_ast(expr);

    return reinterpret_cast<likely_matrix(*)(void)>(functionBuilder.function)();
}
