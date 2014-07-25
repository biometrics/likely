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
#include <llvm/Support/MD5.h>
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
            llvm = PointerType::getUnqual(StructType::create(str->data,
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

    static bool isMat(Type *type)
    {
        // This is safe because matricies are the only struct types created by the backend
        if (PointerType *ptr = dyn_cast<PointerType>(type))
            if (dyn_cast<StructType>(ptr->getElementType()))
                return true;
        return false;
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
    likely_const_expr parent;
    vector<likely_const_expr> subexpressions;

    likely_expression(Value *value = NULL, likely_type type = likely_matrix_void, likely_const_expr parent = NULL)
        : value(value), type(type), parent(parent) {}

    virtual ~likely_expression()
    {
        for (likely_const_expr e : subexpressions)
            delete e;
    }

    virtual int uid() const { return __LINE__; }
    virtual size_t maxParameters() const { return 0; }
    virtual size_t minParameters() const { return maxParameters(); }
    virtual void *symbol() const { return NULL; } // Idiom to ensure that specified library symbols aren't stripped when optimizing executable size

    // Environments never get deleted,
    // but we may wish to temporarily reduce their memory footprint when possible.
    virtual void compress() {}
    virtual void decompress() {}

    bool equals(likely_const_expr other) const
    {
        return (this == other)
               || (this
                   && other
                   && (!parent == !other->parent)
                   && (!parent || parent->equals(other->parent))
                   && (uid() == other->uid())
                   && safeEquals(other));
    }

    virtual bool safeEquals(likely_const_expr other) const
    {
        return value == other->value;
    }

    virtual likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const;

    operator Value*() const { return value; }
    operator likely_type() const { return type; }

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
        env->ast = likely_new_atom(name);
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
        return ast->is_list ? ast->num_atoms : 1;
    }
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

#define TRY_EXPR(BUILDER, AST, EXPR)                                       \
const unique_ptr<const likely_expression> EXPR((BUILDER).expression(AST)); \
if (!EXPR.get()) return NULL;                                              \

} // namespace (anonymous)

struct likely_resources
{
    Module *module;
    vector<likely_const_expr> expressions;
    size_t ref_count = 1;

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
        for (likely_const_expr e : expressions)
            delete e;
        delete module;
    }
};

static likely_res likely_retain_resources(likely_const_res resources)
{
    if (resources) const_cast<likely_res>(resources)->ref_count++;
    return const_cast<likely_res>(resources);
}

static void likely_release_resources(likely_const_res resources)
{
    if (!resources || --const_cast<likely_res>(resources)->ref_count) return;
    delete resources;
}

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
        return cache[currentHash].get() != NULL;
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

    Builder(likely_env env)
        : IRBuilder<>(C), env(likely_retain_env(env)) {}

    ~Builder() { likely_release_env(env); }

    static likely_const_expr getMat(likely_const_expr e)
    {
        if (!e) return NULL;
        if (e->value)
            if (PointerType *type = dyn_cast<PointerType>(e->value->getType()))
                if (isa<StructType>(type->getElementType()))
                    return e;
        return getMat(e->parent);
    }

    likely_expression channels(likely_const_expr e) { likely_const_expr m = getMat(e); return (m && likely_multi_channel(*m)) ? likely_expression(CreateLoad(CreateStructGEP(*m, 2), "channels"), likely_matrix_native) : likely_expression::one(); }
    likely_expression columns (likely_const_expr e) { likely_const_expr m = getMat(e); return (m && likely_multi_column (*m)) ? likely_expression(CreateLoad(CreateStructGEP(*m, 3), "columns" ), likely_matrix_native) : likely_expression::one(); }
    likely_expression rows    (likely_const_expr e) { likely_const_expr m = getMat(e); return (m && likely_multi_row    (*m)) ? likely_expression(CreateLoad(CreateStructGEP(*m, 4), "rows"    ), likely_matrix_native) : likely_expression::one(); }
    likely_expression frames  (likely_const_expr e) { likely_const_expr m = getMat(e); return (m && likely_multi_frame  (*m)) ? likely_expression(CreateLoad(CreateStructGEP(*m, 5), "frames"  ), likely_matrix_native) : likely_expression::one(); }
    likely_expression data    (likely_const_expr e) { likely_const_expr m = getMat(e); return likely_expression(CreatePointerCast(CreateStructGEP(*m, 7), MatType::scalar(*m, true)), likely_data(*m)); }

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
        Type *dstType = MatType::scalar(type);
        return likely_expression(CreateCast(CastInst::getCastOpcode(*x, likely_signed(*x), dstType, likely_signed(type)), *x, dstType), type);
    }

    likely_const_expr lookup(const char *name) const { return likely_expression::lookup(env, name); }
    void define(const char *name, likely_const_expr e) { likely_expression::define(env, name, e); }
    likely_const_expr undefine(const char *name) { return likely_expression::undefine(env, name); }

    void undefineAll(likely_const_ast args, bool deleteExpression)
    {
        if (args->is_list) {
            for (size_t i=0; i<args->num_atoms; i++) {
                likely_const_expr expression = undefine(args->atoms[args->num_atoms-i-1]->atom);
                if (deleteExpression) delete expression;
            }
        } else {
            likely_const_expr expression = undefine(args->atom);
            if (deleteExpression) delete expression;
        }
    }

    Module *module() { return env->resources->module; }

    likely_const_expr expression(likely_const_ast ast);
};

struct Symbol : public likely_expression
{
    Symbol(Function *function = NULL, likely_type type = likely_matrix_void)
        : likely_expression(function, type) {}

private:
    int uid() const { return __LINE__; }

    bool safeEquals(likely_const_expr other) const
    {
        Function *function = dyn_cast_or_null<Function>(value);
        Function *otherFunction = dyn_cast_or_null<Function>(other->value);
        return function && otherFunction && (function->getName() == otherFunction->getName());
    }

    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
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

struct JITFunction : public likely_function, public Symbol
{
    ExecutionEngine *EE = NULL;
    likely_res resources;
    hash_code hash = 0;
    const vector<likely_type> parameters;

    JITFunction(const string &name, likely_const_ast ast, likely_const_env env, const vector<likely_type> &parameters, bool interpreter, bool arrayCC);

    ~JITFunction()
    {
        resources->module = NULL;
        likely_release_resources(resources);
        delete EE; // owns module
    }

private:
    int uid() const { return __LINE__; }

    bool safeEquals(likely_const_expr other) const
    {
        return EE == static_cast<const JITFunction*>(other)->EE;
    }
};

class Operator : public likely_expression
{
    likely_const_expr evaluate(Builder &builder, likely_const_ast ast) const
    {
        if (!ast->is_list && (minParameters() > 0))
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
        : env(likely_retain_env(env)), ast(likely_retain_ast(ast)) {}

    ~ScopedExpression()
    {
        likely_release_ast(ast);
        likely_release_env(env);
    }

private:
    bool safeEquals(likely_const_expr other) const
    {
        return (env == static_cast<const ScopedExpression*>(other)->env) &&
               !likely_ast_compare(ast, static_cast<const ScopedExpression*>(other)->ast);
    }
};

} // namespace (anonymous)

likely_const_expr likely_expression::evaluate(Builder &builder, likely_const_ast ast) const
{
    if (ast->is_list) {
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
    int uid() const { return __LINE__; }
    bool safeEquals(likely_const_expr other) const { return this == other; }
    likely_const_expr evaluateOperator(Builder &, likely_const_ast) const { return NULL; }
};

namespace {

static int getPrecedence(const char *op)
{
    if (!strcmp(op, "=" )) return 1;
    if (!strcmp(op, "->")) return 2;
    if (!strcmp(op, "=>")) return 2;
    if (!strcmp(op, "#" )) return 3;
    if (!strcmp(op, "?" )) return 3;
    if (!strcmp(op, "$" )) return 3;
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

struct RootEnvironment
{
    // Provide public access to an environment that includes the standard library.
    static likely_env get()
    {
        static bool init = false;
        if (!init) {
            builtins() = likely_repl(likely_standard_library, true, builtins(), NULL);
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
        if (int precedence = getPrecedence(symbol))
            likely_insert_operator(symbol, precedence, int(e->minParameters())-1);
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
    MatrixType(likely_type t)
        : t(t)
    {
        value = typeType(t);
        type = likely_matrix_type_type;
    }

private:
    int uid() const { return __LINE__; }
    bool safeEquals(likely_const_expr other) const { return t == static_cast<const MatrixType*>(other)->t; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &x) const
    {
        return new likely_expression(builder.cast(x.get(), t));
    }
};

likely_const_expr Builder::expression(likely_const_ast ast)
{
    if (ast->is_list) {
        if (ast->num_atoms == 0)
            return likely_expression::error(ast, "Empty expression");
        likely_const_ast op = ast->atoms[0];
        if (!op->is_list)
            if (likely_const_expr e = lookup(op->atom))
                return e->evaluate(*this, ast);
        TRY_EXPR(*this, op, e);
        return e->evaluate(*this, ast);
    }
    const string op = ast->atom;

    if (likely_const_expr e = lookup(op.c_str()))
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

#define LIKELY_REGISTER_FIELD(FIELD, UID)                                                                         \
class FIELD##Expression : public SimpleUnaryOperator                                                              \
{                                                                                                                 \
    int uid() const { return UID; }                                                                               \
                                                                                                                  \
    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const \
    {                                                                                                             \
        return new likely_expression(builder.FIELD(arg.get()));                                                   \
    }                                                                                                             \
};                                                                                                                \
LIKELY_REGISTER(FIELD)                                                                                            \

LIKELY_REGISTER_FIELD(channels, __LINE__)
LIKELY_REGISTER_FIELD(columns , __LINE__)
LIKELY_REGISTER_FIELD(rows    , __LINE__)
LIKELY_REGISTER_FIELD(frames  , __LINE__)

class notExpression : public SimpleUnaryOperator
{
    int uid() const { return __LINE__; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(builder.CreateXor(intMax(*arg), arg->value), *arg);
    }
};
LIKELY_REGISTER_EXPRESSION(not, "~")

class typeExpression : public SimpleUnaryOperator
{
    int uid() const { return __LINE__; }

    likely_const_expr evaluateSimpleUnary(Builder &, const unique_ptr<const likely_expression> &arg) const
    {
        return new MatrixType(*arg);
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

#define LIKELY_REGISTER_UNARY_MATH(OP, UID)            \
class OP##Expression : public UnaryMathOperator        \
{                                                      \
    int uid() const { return UID; }                    \
    Intrinsic::ID id() const { return Intrinsic::OP; } \
};                                                     \
LIKELY_REGISTER(OP)                                    \

LIKELY_REGISTER_UNARY_MATH(sqrt     , __LINE__)
LIKELY_REGISTER_UNARY_MATH(sin      , __LINE__)
LIKELY_REGISTER_UNARY_MATH(cos      , __LINE__)
LIKELY_REGISTER_UNARY_MATH(exp      , __LINE__)
LIKELY_REGISTER_UNARY_MATH(exp2     , __LINE__)
LIKELY_REGISTER_UNARY_MATH(log      , __LINE__)
LIKELY_REGISTER_UNARY_MATH(log10    , __LINE__)
LIKELY_REGISTER_UNARY_MATH(log2     , __LINE__)
LIKELY_REGISTER_UNARY_MATH(floor    , __LINE__)
LIKELY_REGISTER_UNARY_MATH(ceil     , __LINE__)
LIKELY_REGISTER_UNARY_MATH(trunc    , __LINE__)
LIKELY_REGISTER_UNARY_MATH(rint     , __LINE__)
LIKELY_REGISTER_UNARY_MATH(nearbyint, __LINE__)
LIKELY_REGISTER_UNARY_MATH(round    , __LINE__)

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
    int uid() const { return __LINE__; }

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
    int uid() const { return __LINE__; }

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
    int uid() const { return __LINE__; }

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
    int uid() const { return __LINE__; }

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

class remainderExpression : public SimpleArithmeticOperator
{
    int uid() const { return __LINE__; }

    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const
    {
        return likely_floating(lhs) ? builder.CreateFRem(lhs, rhs)
                                    : (likely_signed(lhs) ? builder.CreateSRem(lhs, rhs)
                                                          : builder.CreateURem(lhs, rhs));
    }
};
LIKELY_REGISTER_EXPRESSION(remainder, "%")

#define LIKELY_REGISTER_LOGIC(OP, SYM, UID)                                                                             \
class OP##Expression : public SimpleArithmeticOperator                                                                  \
{                                                                                                                       \
    int uid() const { return UID; }                                                                                     \
                                                                                                                        \
    Value *evaluateSimpleArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const \
    {                                                                                                                   \
        return builder.Create##OP(lhs, rhs.value);                                                                      \
    }                                                                                                                   \
};                                                                                                                      \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                     \

LIKELY_REGISTER_LOGIC(And , "&"   , __LINE__)
LIKELY_REGISTER_LOGIC(Or  , "|"   , __LINE__)
LIKELY_REGISTER_LOGIC(Xor , "^"   , __LINE__)
LIKELY_REGISTER_LOGIC(Shl , "<<"  , __LINE__)
LIKELY_REGISTER_LOGIC(LShr, "lshr", __LINE__)
LIKELY_REGISTER_LOGIC(AShr, "ashr", __LINE__)

#define LIKELY_REGISTER_COMPARISON(OP, SYM, UID)                                                                                         \
class OP##Expression : public ArithmeticOperator                                                                                         \
{                                                                                                                                        \
    int uid() const { return UID; }                                                                                                      \
                                                                                                                                         \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const             \
    {                                                                                                                                    \
        return new likely_expression(likely_floating(lhs) ? builder.CreateFCmpO##OP(lhs, rhs)                                            \
                                                          : (likely_signed(lhs) ? builder.CreateICmpS##OP(lhs, rhs)                      \
                                                                                : builder.CreateICmpU##OP(lhs, rhs)), likely_matrix_u1); \
    }                                                                                                                                    \
};                                                                                                                                       \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                                      \

LIKELY_REGISTER_COMPARISON(LT, "<" , __LINE__)
LIKELY_REGISTER_COMPARISON(LE, "<=", __LINE__)
LIKELY_REGISTER_COMPARISON(GT, ">" , __LINE__)
LIKELY_REGISTER_COMPARISON(GE, ">=", __LINE__)

#define LIKELY_REGISTER_EQUALITY(OP, SYM, UID)                                                                               \
class OP##Expression : public ArithmeticOperator                                                                             \
{                                                                                                                            \
    int uid() const { return UID; }                                                                                          \
                                                                                                                             \
    likely_const_expr evaluateArithmetic(Builder &builder, const likely_expression &lhs, const likely_expression &rhs) const \
    {                                                                                                                        \
        return new likely_expression(likely_floating(lhs) ? builder.CreateFCmpO##OP(lhs, rhs)                                \
                                                          : builder.CreateICmp##OP(lhs, rhs), likely_matrix_u1);             \
    }                                                                                                                        \
};                                                                                                                           \
LIKELY_REGISTER_EXPRESSION(OP, SYM)                                                                                          \

LIKELY_REGISTER_EQUALITY(EQ, "==", __LINE__)
LIKELY_REGISTER_EQUALITY(NE, "!=", __LINE__)

class BinaryMathOperator : public SimpleBinaryOperator
{
    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &x, const unique_ptr<const likely_expression> &n) const
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
    int uid() const { return __LINE__; }
    Intrinsic::ID id() const { return Intrinsic::powi; }
    bool nIsInteger() const { return true; }
};
LIKELY_REGISTER(powi)

#define LIKELY_REGISTER_BINARY_MATH(OP, UID)           \
class OP##Expression : public BinaryMathOperator       \
{                                                      \
    int uid() const { return UID; }                    \
    Intrinsic::ID id() const { return Intrinsic::OP; } \
};                                                     \
LIKELY_REGISTER(OP)                                    \

LIKELY_REGISTER_BINARY_MATH(pow     , __LINE__)
LIKELY_REGISTER_BINARY_MATH(copysign, __LINE__)

class definedExpression : public Operator
{
    int uid() const { return __LINE__; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
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
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_elements; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
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
};
LIKELY_REGISTER(elements)

class bytesExpression : public SimpleUnaryOperator
{
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_bytes; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        static FunctionType *functionType = FunctionType::get(NativeInt, MatType::MultiDimension, false);
        Function *likelyBytes = builder.module()->getFunction("likely_bytes");
        if (!likelyBytes) {
            likelyBytes = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_bytes", builder.module());
            likelyBytes->setCallingConv(CallingConv::C);
            likelyBytes->setDoesNotAlias(1);
            likelyBytes->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyBytes, builder.CreatePointerCast(*arg, MatType::MultiDimension)), likely_matrix_native);
    }
};
LIKELY_REGISTER(bytes)

class newExpression : public Operator
{
    int uid() const { return __LINE__; }
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

public:
    static CallInst *createCall(Builder &builder, Value *type, Value *channels, Value *columns, Value *rows, Value *frames, Value *data)
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type *params[] = { NativeInt, NativeInt, NativeInt, NativeInt, NativeInt, Type::getInt8PtrTy(C) };
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
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_scalar_va; }

    likely_const_expr evaluateUnary(Builder &builder, likely_const_ast arg) const
    {
        likely_const_expr argExpr = builder.expression(arg);
        if (!argExpr)
            return NULL;

        if (argExpr->value && (argExpr->value->getType() == MatType::MultiDimension))
            return argExpr;

        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type *params[] = { NativeInt, Type::getDoubleTy(C) };
            functionType = FunctionType::get(MatType::MultiDimension, params, true);
            sys::DynamicLibrary::AddSymbol("lle_X_likely_scalar_va", (void*) lle_X_likely_scalar_va);
        }

        Function *likelyScalar = builder.module()->getFunction("likely_scalar_va");
        if (!likelyScalar) {
            likelyScalar = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_scalar_va", builder.module());
            likelyScalar->setCallingConv(CallingConv::C);
            likelyScalar->setDoesNotAlias(0);
        }

        vector<Value*> args;
        likely_type type = likely_matrix_void;
        for (likely_const_expr e : argExpr->subexpressionsOrSelf()) {
            args.push_back(builder.cast(e, likely_matrix_f64));
            type = likely_type_from_types(type, e->type);
        }
        args.push_back(ConstantFP::get(C, APFloat::getNaN(APFloat::IEEEdouble)));
        args.insert(args.begin(), typeType(type));

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
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_string; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), likely_matrix_i8);
    }

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
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_copy; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), *arg);
    }

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
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_retain; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), *arg);
    }

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
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_release; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        return new likely_expression(createCall(builder, *arg), *arg);
    }

    // This is not publicly accessible because cleanup() is the only time release statements should be inserted
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
                       if (MatType::isMat(function->getReturnType()) && (call != ret))
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
        if (ast->is_list && (ast->num_atoms > 1))
            if (ast->atoms[1]->is_list) n = ast->atoms[1]->num_atoms;
            else                        n = 1;
        else                            n = 0;

        while (types.size() < n)
            types.push_back(MatType::MultiDimension);

        vector<Type*> llvmTypes;
        if (arrayCC) {
            // Array calling convention - All arguments, which must be matrix pointers, come stored in an array for the convenience of likely_dynamic.
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

        likely_const_expr result = evaluateFunction(builder, arguments);
        if (!result) return NULL;

        releaseExpression::cleanup(builder, result->value);

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
    int uid() const { return __LINE__; }
    size_t maxParameters() const { return length(ast->atoms[1]); }
    void *symbol() const { return (void*) likely_dynamic; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        vector<likely_expression> args;
        const size_t arguments = length(ast)-1;
        for (size_t i=0; i<arguments; i++) {
            TRY_EXPR(builder, ast->atoms[i+1], arg)
            args.push_back(likely_expression(arg->value, arg->type));
        }
        return evaluateFunction(builder, args);
    }

    likely_const_expr evaluateFunction(Builder &builder, const vector<likely_expression> &args) const
    {
        assert(args.size() == length(ast->atoms[1]));

        // Do dynamic dispatch if the type isn't fully specified
        bool dynamic = false;
        for (const likely_expression &arg : args)
            dynamic |= (arg.type == MatType::MultiDimension);

        if (dynamic) {
            likely_vtable vtable = new likely_virtual_table(builder.env, ast);
            builder.env->resources->expressions.push_back(vtable);

            static PointerType *vTableType = PointerType::getUnqual(StructType::create(C, "VTable"));
            static FunctionType *likelyDynamicType = NULL;
            if (likelyDynamicType == NULL) {
                Type *params[] = { vTableType, PointerType::get(MatType::MultiDimension, 0) };
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

    virtual likely_const_expr evaluateLambda(Builder &builder, const vector<likely_expression> &args) const
    {
        if (ast->atoms[1]->is_list) {
            for (size_t i=0; i<args.size(); i++)
                builder.define(ast->atoms[1]->atoms[i]->atom, new likely_expression(args[i]));
        } else {
            builder.define(ast->atoms[1]->atom, new likely_expression(args[0]));
        }
        likely_const_expr result = builder.expression(ast->atoms[2]);
        builder.undefineAll(ast->atoms[1], true);
        return result;
    }
};

class lambdaExpression : public Operator
{
    int uid() const { return __LINE__; }
    size_t maxParameters() const { return 2; }
    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const { return new Lambda(builder.env, ast); }

public:
    static bool isLambda(likely_const_ast ast)
    {
        return ast->is_list
               && (ast->num_atoms > 0)
               && !ast->atoms[0]->is_list
               && (!strcmp(ast->atoms[0]->atom, "->") || !strcmp(ast->atoms[0]->atom, "=>"));
    }
};
LIKELY_REGISTER_EXPRESSION(lambda, "->")

class beginExpression : public Operator
{
    int uid() const { return __LINE__; }
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

class indexExpression : public Operator
{
    int uid() const { return __LINE__; }
    size_t minParameters() const { return 1; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[1], expr)
        likely_expr result = new likely_expression();
        for (size_t i=2; i<ast->num_atoms; i++) {
            const size_t index = atoi(ast->atoms[i]->atom);
            if (index < expr->subexpressions.size()) {
                likely_const_expr e = NULL;
                swap(e, const_cast<likely_expr>(expr.get())->subexpressions[index]);
                result->subexpressions.push_back(e);
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
    int uid() const { return __LINE__; }

    likely_const_expr evaluate(Builder &builder, likely_const_ast) const
    {
        BasicBlock *basicBlock = cast<BasicBlock>(value);
        builder.CreateBr(basicBlock);
        return new Label(basicBlock);
    }
};

class labelExpression : public Operator
{
    int uid() const { return __LINE__; }
    size_t maxParameters() const { return 1; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        const string name = ast->atoms[1]->atom;
        BasicBlock *label = BasicBlock::Create(C, name, builder.GetInsertBlock()->getParent());
        builder.CreateBr(label);
        builder.SetInsertPoint(label);
        builder.define(name.c_str(), new Label(label));
        return new Label(label);
    }
};
LIKELY_REGISTER_EXPRESSION(label, "#")

class ifExpression : public Operator
{
    int uid() const { return __LINE__; }
    size_t minParameters() const { return 2; }
    size_t maxParameters() const { return 3; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
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

private:
    int uid() const { return __LINE__; }
};

class loopExpression : public Operator
{
    int uid() const { return __LINE__; }
    size_t maxParameters() const { return 3; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        TRY_EXPR(builder, ast->atoms[3], end)
        Loop loop(builder, ast->atoms[2]->atom, zero(), *end);
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
        int uid() const { return __LINE__; }
        size_t maxParameters() const { return 0; }

        likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
        {
            if (ast->is_list)
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
                i = zero();
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

    private:
        int uid() const { return __LINE__; }
    };

    struct Metadata
    {
        set<string> collapsedAxis;
        size_t results;
    };

    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_fork; }

    virtual likely_const_ast getMetadata() const { return (ast->num_atoms == 4) ? ast->atoms[3] : NULL; }

    likely_const_expr evaluateLambda(Builder &builder, const vector<likely_expression> &srcs) const
    {
        likely_type kernelType = likely_matrix_void;
        if (!srcs.empty())
            likely_set_execution(&kernelType, likely_execution(srcs.front()));

        vector<pair<likely_const_ast,likely_const_ast>> pairs;
        getPairs(getMetadata(), pairs);

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

        // Allocate and initialize memory for the destination matrix
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

        // Load scalar values
        BasicBlock *scalarMatrixPromotion = BasicBlock::Create(C, "scalar_matrix_promotion", builder.GetInsertBlock()->getParent());
        builder.CreateBr(scalarMatrixPromotion);
        builder.SetInsertPoint(scalarMatrixPromotion);
        vector<likely_expression> thunkSrcs = srcs;
        for (likely_expression &thunkSrc : thunkSrcs)
            if (!likely_multi_dimension(thunkSrc) && MatType::isMat(thunkSrc.value->getType()))
                thunkSrc = likely_expression(builder.CreateLoad(builder.CreateGEP(builder.data(&thunkSrc), zero())), thunkSrc);

        // Finally, do the computation
        BasicBlock *computation = BasicBlock::Create(C, "computation", builder.GetInsertBlock()->getParent());
        builder.CreateBr(computation);
        builder.SetInsertPoint(computation);

        Metadata metadata;
        if      (likely_heterogeneous(kernelType)) metadata = generateHeterogeneous(builder, args, thunkSrcs, dst, kernelSize);
        else if (likely_parallel(kernelType))      metadata = generateParallel     (builder, args, thunkSrcs, dst, kernelSize);
        else                                       metadata = generateSerial       (builder, args, thunkSrcs, dst, kernelSize);

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
        BasicBlock *entry = builder.GetInsertBlock();

        vector<Type*> parameterTypes;
        for (const likely_expression &src : srcs)
            parameterTypes.push_back(src.value->getType());
        parameterTypes.push_back(dst.value->getType());
        StructType *parameterStructType = StructType::get(C, parameterTypes);

        Function *thunk;
        Metadata metadata;
        {
            Type *params[] = { PointerType::getUnqual(parameterStructType), NativeInt, NativeInt };
            FunctionType *thunkType = FunctionType::get(Type::getVoidTy(C), params, false);

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

            builder.SetInsertPoint(BasicBlock::Create(C, "entry", thunk));
            vector<likely_expression> thunkSrcs;
            for (size_t i=0; i<srcs.size()+1; i++) {
                const likely_type &type = i < srcs.size() ? srcs[i].type : dst.type;
                thunkSrcs.push_back(likely_expression(builder.CreateLoad(builder.CreateStructGEP(parameterStruct, i)), type));
            }
            likely_expression kernelDst = thunkSrcs.back(); thunkSrcs.pop_back();
            kernelDst.value->setName("dst");

            metadata = generateCommon(builder, args, thunkSrcs, kernelDst, start, stop);

            dst.type = kernelDst;
            builder.CreateRetVoid();
        }

        builder.SetInsertPoint(entry);

        Type *params[] = { thunk->getType(), PointerType::getUnqual(parameterStructType), NativeInt };
        FunctionType *likelyForkType = FunctionType::get(Type::getVoidTy(C), params, false);
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
            const likely_expression &src = i < srcs.size() ? srcs[i] : dst;
            builder.CreateStore(src, builder.CreateStructGEP(parameterStruct, i));
        }

        builder.CreateCall3(likelyFork, builder.module()->getFunction(thunk->getName()), parameterStruct, kernelSize);
        return metadata;
    }

    Metadata generateHeterogeneous(Builder &, likely_const_ast, const vector<likely_expression> &, likely_expression &, Value *) const
    {
        assert(!"Not implemented");
        return Metadata();
    }

    Metadata generateCommon(Builder &builder, likely_const_ast args, const vector<likely_expression> &srcs, likely_expression &dst, Value *start, Value *stop) const
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

        unique_ptr<const likely_expression> result(builder.expression(ast->atoms[2]));

        const vector<likely_const_expr> expressions = result->subexpressionsOrSelf();
        for (likely_const_expr e : expressions)
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

    static bool getPairs(likely_const_ast ast, vector<pair<likely_const_ast,likely_const_ast>> &pairs)
    {
        pairs.clear();
        if (!ast) return true;
        if (!ast->is_list) return false;
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
                result = builder.cast(unique_ptr<const likely_expression>(builder.expression(pair.second)).get(), likely_matrix_native);
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
    int uid() const { return __LINE__; }
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
    int uid() const { return __LINE__; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        unique_ptr<const likely_expression> op(builder.expression(this->ast));
        return op.get() ? op->evaluate(builder, ast) : NULL;
    }

    size_t minParameters() const { return 0; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }
};

struct EvaluatedExpression : public ScopedExpression
{
    likely_env result;

    EvaluatedExpression(likely_env env, likely_const_ast ast)
        : ScopedExpression(env, ast)

    {
        result = likely_eval(ast, env, NULL);
        assert(!likely_definition(result->type) && result->result);
    }

    ~EvaluatedExpression()
    {
        likely_release_env(result);
    }

private:
    int uid() const { return __LINE__; }

    void compress()
    {
        likely_release_env(result);
    }

    void decompress()
    {
        likely_retain_env(result);
    }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        // TODO: implement indexing into this matrix by checking ast.
        // Consider sharing implementation with kernelArgument.
        (void) builder;
        (void) ast;

        likely_const_mat m = result->result;
        if (likely_elements(m) == 1) {
            // Promote to scalar
            return new likely_expression(constant(likely_element(m, 0, 0, 0, 0), m->type));
        } else {
            // Return the matrix
            return new likely_expression(ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(C, 8*sizeof(m)), uintptr_t(m)), MatType::get(m->type)), m->type);
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
    int uid() const { return __LINE__; }
    size_t maxParameters() const { return 2; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        likely_const_ast lhs = ast->atoms[1];
        likely_const_ast rhs = ast->atoms[2];
        const char *name = lhs->is_list ? lhs->atoms[0]->atom : lhs->atom;
        likely_env env = builder.env;

        if (likely_global(env->type)) {
            assert(!env->value);
            if (lhs->is_list) {
                // Export symbol
                vector<likely_type> types;
                for (size_t i=1; i<lhs->num_atoms; i++) {
                    if (lhs->atoms[i]->is_list)
                        return error(lhs->atoms[i], "expected an atom name parameter type");
                    types.push_back(likely_type_from_string(lhs->atoms[i]->atom));
                }

                if (likely_offline(env->type)) {
                    TRY_EXPR(builder, rhs, expr);
                    const Lambda *lambda = static_cast<const Lambda*>(expr.get());
                    env->value = lambda->generate(builder, types, name, false);
                } else {
                    JITFunction *function = new JITFunction(name, rhs, builder.env, types, false, false);
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
    int uid() const { return __LINE__; }
    size_t maxParameters() const { return 1; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
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

JITFunction::JITFunction(const string &name, likely_const_ast ast, likely_const_env parent, const vector<likely_type> &parameters, bool interpreter, bool arrayCC)
    : resources(new likely_resources(true)), parameters(parameters)
{
    function = NULL;
    ref_count = 1;

    if (!lambdaExpression::isLambda(ast)) {
        likely_expression::error(ast, "expected a lambda expression");
        return;
    }

    {
        Builder builder(likely_new_env(parent));
        swap(builder.env->resources, resources);
        unique_ptr<const likely_expression> result(builder.expression(ast));
        unique_ptr<const Symbol> expr(static_cast<const Lambda*>(result.get())->generate(builder, parameters, name, arrayCC));
        swap(builder.env->resources, resources);
        likely_release_env(builder.env);
        value = expr->value;
        type = expr->type;
    }

    if (!value)
        return;

    TargetMachine *targetMachine = likely_resources::getTargetMachine(true);
    resources->module->setDataLayout(targetMachine->getDataLayout());

    string error;
    EngineBuilder engineBuilder(resources->module);
    engineBuilder.setErrorStr(&error);

    if (interpreter) {
        engineBuilder.setEngineKind(EngineKind::Interpreter);
    } else {
        engineBuilder.setEngineKind(EngineKind::JIT)
                     .setUseMCJIT(true);
    }

    EE = engineBuilder.create(targetMachine);
    likely_assert(EE != NULL, "failed to create execution engine with error: %s", error.c_str());

    if (interpreter)
        return;

    EE->setObjectCache(&TheJITFunctionCache);
    if (!TheJITFunctionCache.alert(resources->module)) {
        static PassManager *PM = NULL;
        if (!PM) {
            static TargetMachine *TM = likely_resources::getTargetMachine(false);
            PM = new PassManager();
            PM->add(createVerifierPass());
            PM->add(new TargetLibraryInfo(Triple(resources->module->getTargetTriple())));
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
//        resources->module->dump();
        PM->run(*resources->module);
//        resources->module->dump();
    }

    hash = TheJITFunctionCache.currentHash;
    EE->finalizeObject();
    function = (void*) EE->getFunctionAddress(name);
}

#ifdef LIKELY_IO
#include "likely/io.h"

class printExpression : public Operator
{
    int uid() const { return __LINE__; }
    size_t minParameters() const { return 1; }
    size_t maxParameters() const { return numeric_limits<size_t>::max(); }
    void *symbol() const { return (void*) likely_print_va; }

    likely_const_expr evaluateOperator(Builder &builder, likely_const_ast ast) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            functionType = FunctionType::get(MatType::MultiDimension, MatType::MultiDimension, true);
            sys::DynamicLibrary::AddSymbol("lle_X_likely_print_va", (void*) lle_X_likely_print_va);
        }

        Function *likelyPrint = builder.module()->getFunction("likely_print_va");
        if (!likelyPrint) {
            likelyPrint = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_print_va", builder.module());
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
                // Intermediate matricies will be released when this function returns
                matArgs.push_back(stringExpression::createCall(builder, rawArg));
            }

        Value *result = builder.CreateCall(likelyPrint, matArgs);

        return new likely_expression(result, likely_matrix_i8);
    }

    static GenericValue lle_X_likely_print_va(FunctionType *, const vector<GenericValue> &Args)
    {
        vector<likely_const_mat> mv;
        for (size_t i=1; i<Args.size()-1; i++)
            mv.push_back((likely_const_mat) Args[i].PointerVal);
        likely_print_n(mv.data(), mv.size());
        return GenericValue(0);
    }
};
LIKELY_REGISTER(print)

class readExpression : public SimpleUnaryOperator
{
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_read; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type *params[] = { Type::getInt8PtrTy(C), NativeInt };
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
        return new likely_expression(builder.CreateCall2(likelyRead, *arg, constant(likely_file_binary)), MatType::MultiDimension);
    }
};
LIKELY_REGISTER(read)

class writeExpression : public SimpleBinaryOperator
{
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_write; }

    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &arg1, const unique_ptr<const likely_expression> &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type *params[] = { MatType::MultiDimension, Type::getInt8PtrTy(C) };
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
};
LIKELY_REGISTER(write)

class decodeExpression : public SimpleUnaryOperator
{
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_decode; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
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
        return new likely_expression(builder.CreateCall(likelyDecode, builder.CreatePointerCast(*arg, MatType::MultiDimension)), MatType::MultiDimension);
    }
};
LIKELY_REGISTER(decode)

class encodeExpression : public SimpleBinaryOperator
{
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_encode; }

    likely_const_expr evaluateSimpleBinary(Builder &builder, const unique_ptr<const likely_expression> &arg1, const unique_ptr<const likely_expression> &arg2) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type *params[] = { MatType::MultiDimension, Type::getInt8PtrTy(C) };
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
        return new likely_expression(builder.CreateCall2(likelyEncode, builder.CreatePointerCast(*arg1, MatType::MultiDimension), *arg2), MatType::MultiDimension);
    }
};
LIKELY_REGISTER(encode)

class renderExpression : public SimpleUnaryOperator
{
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_render; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type *params[] = { MatType::MultiDimension, Type::getDoublePtrTy(C), Type::getDoublePtrTy(C) };
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
};
LIKELY_REGISTER(render)

class showExpression : public SimpleUnaryOperator
{
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_decode; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL) {
            Type *params[] = { MatType::MultiDimension, Type::getInt8PtrTy(C) };
            functionType = FunctionType::get(Type::getVoidTy(C), params, false);
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
};
LIKELY_REGISTER(show)
#endif // LIKELY_IO

class md5Expression : public SimpleUnaryOperator
{
    int uid() const { return __LINE__; }
    void *symbol() const { return (void*) likely_md5; }

    likely_const_expr evaluateSimpleUnary(Builder &builder, const unique_ptr<const likely_expression> &arg) const
    {
        static FunctionType *functionType = NULL;
        if (functionType == NULL)
            functionType = FunctionType::get(MatType::MultiDimension, MatType::MultiDimension, false);
        Function *likelyMd5 = builder.module()->getFunction("likely_md5");
        if (!likelyMd5) {
            likelyMd5 = Function::Create(functionType, GlobalValue::ExternalLinkage, "likely_md5", builder.module());
            likelyMd5->setCallingConv(CallingConv::C);
            likelyMd5->setDoesNotAlias(0);
            likelyMd5->setDoesNotAlias(1);
            likelyMd5->setDoesNotCapture(1);
        }
        return new likely_expression(builder.CreateCall(likelyMd5, *arg), MatType::MultiDimension);
    }
};
LIKELY_REGISTER(md5)

} // namespace (anonymous)

likely_env likely_new_env(likely_const_env parent)
{
    likely_env env = new likely_environment();
    env->type = likely_environment_void;
    likely_set_offline(&env->type, parent ? likely_offline(parent->type) : false);
    env->parent = likely_retain_env(parent);
    env->ast = NULL;
    env->resources = likely_retain_resources(parent ? parent->resources : NULL);
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

likely_env likely_new_env_offline(const char *file_name, bool native)
{
    likely_env env = likely_new_env(RootEnvironment::get());
    assert(!env->resources);
    env->resources = new OfflineResources(file_name, native);
    likely_set_offline(&env->type, true);
    return env;
}

likely_env likely_retain_env(likely_const_env env)
{
    if (env) const_cast<likely_env>(env)->ref_count++;
    return const_cast<likely_env>(env);
}

void likely_release_env(likely_const_env env)
{
    if (!env || --const_cast<likely_env>(env)->ref_count) return;

    likely_release_env(env->parent);
    likely_release_ast(env->ast);
    likely_release_resources(env->resources);

    if (likely_global(env->type)) {
        // Global environment variables are guaranteed to be unique, so we never delete them.
        // Instead we only deallocate some of their internals, which can be recomputed if needed.
        if (likely_definition(env->type)) {
            if (env->value)
                const_cast<likely_expr>(env->value)->compress();
        } else {
            likely_release(env->result);
            const_cast<likely_env>(env)->result = NULL;
        }
    } else {
        if (env->value) {
            assert(likely_definition(env->type));
            delete env->value;
        }
        assert(!env->children);
        delete env;
    }
}

bool likely_offline(likely_environment_type type) { return likely_get_bool(type, likely_environment_offline); }
void likely_set_offline(likely_environment_type *type, bool offline) { likely_set_bool(type, offline, likely_environment_offline); }
bool likely_erratum(likely_environment_type type) { return likely_get_bool(type, likely_environment_erratum); }
void likely_set_erratum(likely_environment_type *type, bool erratum) { likely_set_bool(type, erratum, likely_environment_erratum); }
bool likely_definition(likely_environment_type type) { return likely_get_bool(type, likely_environment_definition); }
void likely_set_definition(likely_environment_type *type, bool definition) { likely_set_bool(type, definition, likely_environment_definition); }
bool likely_global(likely_environment_type type) { return likely_get_bool(type, likely_environment_global); }
void likely_set_global(likely_environment_type *type, bool global) { likely_set_bool(type, global, likely_environment_global); }

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
        vtable->functions.push_back(unique_ptr<JITFunction>(new JITFunction("likely_vtable_entry", vtable->ast, vtable->env, types, false, true)));
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
    return static_cast<likely_fun>(new JITFunction("likely_jit_function", ast, env, types, false, false));
}

likely_fun likely_retain_function(likely_const_fun f)
{
    if (f) const_cast<likely_fun>(f)->ref_count++;
    return const_cast<likely_fun>(f);
}

void likely_release_function(likely_const_fun f)
{
    if (!f || --const_cast<likely_fun>(f)->ref_count) return;
    delete static_cast<const JITFunction*>(f);
}

likely_env likely_eval(likely_const_ast ast, likely_const_env parent, likely_const_env previous)
{
    if (!ast || !parent)
        return NULL;

    likely_env env = likely_new_env(parent);
    likely_set_definition(&env->type, ast->is_list && (ast->num_atoms > 0) && !strcmp(ast->atoms[0]->atom, "="));
    likely_set_global(&env->type, true);
    env->ast = likely_retain_ast(ast);

    if (likely_definition(env->type)) {
        if (previous) {
            // Check against previous environment for precomputed result
            while ((previous != NULL) && (previous != parent))
                previous = previous->parent;

            if (previous) {
                // TODO: check iter->children
            }

            // We reallocate space for more children when our power-of-two sized buffer is full
            //            if ((parent->num_children == 0) || !(parent->num_children & (parent->num_children - 1)))
            //                parent->children = (likely_const_env *) realloc(parent->children, parent->num_children == 0 ? 1 : 2 * parent->num_children);
            //            parent->children[parent->num_children++] = env;
        }

        assert(!Builder(env).expression(ast));
    } else {
        likely_const_ast lambda = likely_ast_from_string("() -> (scalar <ast>)", false);
        likely_release_ast(lambda->atoms[0]->atoms[2]->atoms[1]); // <ast>
        lambda->atoms[0]->atoms[2]->atoms[1] = likely_retain_ast(ast);

        // TODO: Re-enable interpreter for OS X and Ubuntu when intrinsic lowering patch lands
        JITFunction jit("likely_jit_function", lambda->atoms[0], env, vector<likely_type>(), false, false);
        if (jit.function) {
            env->result = reinterpret_cast<likely_function_0>(jit.function)();
        } else if (jit.EE) {
            env->result = (likely_mat) jit.EE->runFunction(cast<Function>(jit.value), vector<GenericValue>()).PointerVal;
        } else {
            likely_set_erratum(&env->type, true);
        }

        likely_release_ast(lambda);
    }

    return env;
}

static likely_repl_callback ReplCallback = NULL;
static void *ReplContext = NULL;

void likely_set_repl_callback(likely_repl_callback callback, void *context)
{
    ReplCallback = callback;
    ReplContext = context;
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
        if (!likely_definition(env->type) && env->result && (likely_elements(env->result) > 0) && ReplCallback)
            ReplCallback(env, ReplContext);
        if (likely_erratum(env->type))
            break;
    }

    likely_release_ast(ast);
    return env;
}

likely_mat likely_md5(likely_const_mat buffer)
{
    MD5 md5;
    md5.update(ArrayRef<uint8_t>(reinterpret_cast<const uint8_t*>(buffer->data), likely_bytes(buffer)));
    MD5::MD5Result md5Result;
    md5.final(md5Result);
    return likely_new(likely_matrix_u8, 16, 1, 1, 1, md5Result);
}
