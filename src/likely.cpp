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
#include <llvm/Analysis/Verifier.h>
#include <llvm/Assembly/PrintModulePass.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <atomic>
#include <cstdarg>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <stack>
#include <sstream>
#include <thread>

#include "likely.h"

using namespace llvm;
using namespace std;

#define LIKELY_NUM_ARITIES 4

#define LLVM_VALUE_TO_INT(VALUE) (llvm::cast<Constant>(VALUE)->getUniqueInteger().getZExtValue())

static likely_type likely_type_native = likely_type_null;
static IntegerType *NativeIntegerType = NULL;
static StructType *TheMatrixStruct = NULL;

typedef struct likely_matrix_private
{
    size_t data_bytes;
    int ref_count;
} likely_matrix_private;

void likely_assert(bool condition, const char *format, ...)
{
    if (condition) return;
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, "Likely %s.\n", ap);
    abort();
}

static int likely_get(likely_type type, likely_type_field mask) { return type & mask; }
static void likely_set(likely_type *type, int i, likely_type_field mask) { *type &= ~mask; *type |= i & mask; }
static bool likely_get_bool(likely_type type, likely_type_field mask) { return type & mask; }
static void likely_set_bool(likely_type *type, bool b, likely_type_field mask) { b ? *type |= mask : *type &= ~mask; }

int  likely_depth(likely_type type) { return likely_get(type, likely_type_depth); }
void likely_set_depth(likely_type *type, int depth) { likely_set(type, depth, likely_type_depth); }
bool likely_signed(likely_type type) { return likely_get_bool(type, likely_type_signed); }
void likely_set_signed(likely_type *type, bool signed_) { likely_set_bool(type, signed_, likely_type_signed); }
bool likely_floating(likely_type type) { return likely_get_bool(type, likely_type_floating); }
void likely_set_floating(likely_type *type, bool floating) { likely_set_bool(type, floating, likely_type_floating); }
bool likely_parallel(likely_type type) { return likely_get_bool(type, likely_type_parallel); }
void likely_set_parallel(likely_type *type, bool parallel) { likely_set_bool(type, parallel, likely_type_parallel); }
bool likely_heterogeneous(likely_type type) { return likely_get_bool(type, likely_type_heterogeneous); }
void likely_set_heterogeneous(likely_type *type, bool heterogeneous) { likely_set_bool(type, heterogeneous, likely_type_heterogeneous); }
bool likely_multi_channel(likely_type type) { return likely_get_bool(type, likely_type_multi_channel); }
void likely_set_multi_channel(likely_type *type, bool multi_channel) { likely_set_bool(type, multi_channel, likely_type_multi_channel); }
bool likely_multi_column(likely_type type) { return likely_get_bool(type, likely_type_multi_column); }
void likely_set_multi_column(likely_type *type, bool multi_column) { likely_set_bool(type, multi_column, likely_type_multi_column); }
bool likely_multi_row(likely_type type) { return likely_get_bool(type, likely_type_multi_row); }
void likely_set_multi_row(likely_type *type, bool multi_row) { likely_set_bool(type, multi_row, likely_type_multi_row); }
bool likely_multi_frame(likely_type type) { return likely_get_bool(type, likely_type_multi_frame); }
void likely_set_multi_frame(likely_type *type, bool multi_frame) { likely_set_bool(type, multi_frame, likely_type_multi_frame); }
bool likely_saturation(likely_type type) { return likely_get_bool(type, likely_type_saturation); }
void likely_set_saturation(likely_type *type, bool saturation) { likely_set_bool(type, saturation, likely_type_saturation); }
int  likely_reserved(likely_type type) { return likely_get(type, likely_type_reserved); }
void likely_set_reserved(likely_type *type, int reserved) { likely_set(type, reserved, likely_type_reserved); }

likely_size likely_elements(likely_const_mat m)
{
    return m->channels * m->columns * m->rows * m->frames;
}

likely_size likely_bytes(likely_const_mat m)
{
    return likely_depth(m->type) * likely_elements(m) / 8;
}

// TODO: make this thread_local when compiler support improves
static likely_mat recycledBuffer = NULL;

static likely_data *dataPointer(likely_mat m)
{
    return reinterpret_cast<likely_data*>(uintptr_t(m+1) + sizeof(likely_matrix_private));
}

likely_mat likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, likely_data *data, int8_t copy)
{
    likely_mat m;
    size_t dataBytes = ((data && !copy) ? 0 : uint64_t(likely_depth(type)) * channels * columns * rows * frames / 8);
    const size_t headerBytes = sizeof(likely_matrix) + sizeof(likely_matrix_private);
    if (recycledBuffer) {
        const size_t recycledDataBytes = recycledBuffer->d_ptr->data_bytes;
        if (recycledDataBytes >= dataBytes) { m = recycledBuffer; dataBytes = recycledDataBytes; }
        else                                m = (likely_mat) realloc(recycledBuffer, headerBytes + dataBytes);
        recycledBuffer = NULL;
    } else {
        m = (likely_mat) malloc(headerBytes + dataBytes);
    }

    m->type = type;
    m->channels = channels;
    m->columns = columns;
    m->rows = rows;
    m->frames = frames;

    likely_set_multi_channel(&m->type, channels != 1);
    likely_set_multi_column(&m->type, columns != 1);
    likely_set_multi_row(&m->type, rows != 1);
    likely_set_multi_frame(&m->type, frames != 1);

    assert(alignof(likely_matrix_private) <= alignof(likely_matrix));
    m->d_ptr = reinterpret_cast<likely_matrix_private*>(m+1);
    m->d_ptr->ref_count = 1;
    m->d_ptr->data_bytes = dataBytes;

    if (data && !copy) {
        m->data = data;
    } else {
        m->data = dataPointer(m);
        if (data && copy) memcpy(m->data, data, likely_bytes(m));
    }

    return m;
}

likely_mat likely_copy(likely_const_mat m, int8_t copy_data)
{
    return likely_new(m->type, m->channels, m->columns, m->rows, m->frames, m->data, copy_data);
}

likely_mat likely_retain(likely_mat m)
{
    if (!m) return m;
    ++m->d_ptr->ref_count;
    return m;
}

void likely_release(likely_mat m)
{
    if (!m) return;
    if (--m->d_ptr->ref_count != 0) return;
    const size_t dataBytes = m->d_ptr->data_bytes;
    if (recycledBuffer) {
        if (dataBytes > recycledBuffer->d_ptr->data_bytes)
            swap(m, recycledBuffer);
        free(m);
    } else {
        recycledBuffer = m;
    }
}

const char *likely_type_to_string(likely_type type)
{
    static string typeString; // Provides return value persistence

    stringstream typeStream;
    typeStream << (likely_floating(type) ? "f" : (likely_signed(type) ? "i" : "u"));
    typeStream << likely_depth(type);

    if (likely_parallel(type))       typeStream << "P";
    if (likely_heterogeneous(type))  typeStream << "H";
    if (likely_multi_channel(type)) typeStream << "C";
    if (likely_multi_column(type))  typeStream << "X";
    if (likely_multi_row(type))     typeStream << "Y";
    if (likely_multi_frame(type))   typeStream << "T";
    if (likely_saturation(type))     typeStream << "S";

    typeString = typeStream.str();
    return typeString.c_str();
}

likely_type likely_type_from_string(const char *str)
{
    assert(str);
    likely_type t = likely_type_null;
    const size_t len = strlen(str);
    if (len == 0) return t;

    if (str[0] == 'f') likely_set_floating(&t, true);
    if (str[0] != 'u') likely_set_signed(&t, true);
    int depth = atoi(str+1); // atoi ignores characters after the number
    if (depth == 0) depth = 32;
    likely_set_depth(&t, depth);

    size_t startIndex = 1;
    while ((startIndex < len) && (str[startIndex] >= '0') && (str[startIndex] <= '9'))
        startIndex++;

    for (size_t i=startIndex; i<len; i++) {
        if (str[i] == 'P') likely_set_parallel(&t, true);
        if (str[i] == 'H') likely_set_heterogeneous(&t, true);
        if (str[i] == 'C') likely_set_multi_channel(&t, true);
        if (str[i] == 'X') likely_set_multi_column(&t, true);
        if (str[i] == 'Y') likely_set_multi_row(&t, true);
        if (str[i] == 'T') likely_set_multi_frame(&t, true);
        if (str[i] == 'S') likely_set_saturation(&t, true);
    }

    return t;
}

likely_type likely_type_from_value(double value)
{
    if      (static_cast< uint8_t>(value) == value) return likely_type_u8;
    else if (static_cast<  int8_t>(value) == value) return likely_type_i8;
    else if (static_cast<uint16_t>(value) == value) return likely_type_u16;
    else if (static_cast< int16_t>(value) == value) return likely_type_i16;
    else if (static_cast<uint32_t>(value) == value) return likely_type_u32;
    else if (static_cast< int32_t>(value) == value) return likely_type_i32;
    else if (static_cast<uint64_t>(value) == value) return likely_type_u64;
    else if (static_cast< int64_t>(value) == value) return likely_type_i64;
    else if (static_cast<   float>(value) == value) return likely_type_f32;
    else                                            return likely_type_f64;
}

likely_type likely_type_from_types(likely_type lhs, likely_type rhs)
{
    likely_type result = lhs | rhs;
    likely_set_depth(&result, max(likely_depth(lhs), likely_depth(rhs)));
    return result;
}

struct TypedValue
{
    Value *value;
    likely_type type;
    TypedValue() : value(NULL), type(likely_type_null) {}
    TypedValue(Value *value_, likely_type type_) : value(value_), type(type_) {}
    operator Value*() const { return value; }
    operator likely_type() const { return type; }
};

struct KernelBuilder
{
    Module *m;
    IRBuilder<> *b;
    Function *f;

    struct Loop {
        BasicBlock *body;
        PHINode *i;
        Value *stop;
        MDNode *node;
    };
    stack<Loop> loops;

    KernelBuilder(Module *module, IRBuilder<> *builder, Function *function)
        : m(module), b(builder), f(function)
    {}

    static TypedValue constant(double value, likely_type type = likely_type_native)
    {
        const int depth = likely_depth(type);
        if (likely_floating(type)) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return TypedValue(ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value), type);
            else if (depth == 32) return TypedValue(ConstantFP::get(Type::getFloatTy(getGlobalContext()), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return TypedValue(); }
        } else {
            return TypedValue(Constant::getIntegerValue(Type::getIntNTy(getGlobalContext(), depth), APInt(depth, value)), type);
        }
    }

    static TypedValue zero(int bits = likely_depth(likely_type_native)) { return constant(0, bits); }
    static TypedValue one(int bits = likely_depth(likely_type_native)) { return constant(1, bits); }
    static TypedValue intMax(int bits) { return constant((1 << (bits-1))-1, bits); }
    static TypedValue intMin(int bits) { return constant((1 << (bits-1)), bits); }

    Value *data(const TypedValue &matrix) const { return b->CreatePointerCast(b->CreateLoad(b->CreateStructGEP(matrix, 0), "data"), ty(matrix, true)); }
    Value *channels(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 2), "channels"); }
    Value *columns(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 3), "columns"); }
    Value *rows(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 4), "rows"); }
    Value *frames(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 5), "frames"); }
    Value *type(Value *v) const { return b->CreateLoad(b->CreateStructGEP(v, 6), "type"); }
    Value *type(likely_type type) const { return constant(type, int(sizeof(likely_type)*8)); }

    void setData(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 0)); }
    void setChannels(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 2)); }
    void setColumns(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 3)); }
    void setRows(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 4)); }
    void setFrames(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 5)); }
    void setType(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 6)); }

    Value *get(Value *matrix, int mask) const { return b->CreateAnd(type(matrix), constant(mask, 8*sizeof(likely_type)).value); }
    void set(Value *matrix, int value, int mask) const { setType(matrix, b->CreateOr(b->CreateAnd(type(matrix), constant(~mask, 8*sizeof(likely_type)).value), b->CreateAnd(constant(value, 8*sizeof(likely_type)), constant(mask, 8*sizeof(likely_type)).value))); }
    void setBit(Value *matrix, bool on, int mask) const { on ? setType(matrix, b->CreateOr(type(matrix), constant(mask, 8*sizeof(likely_type)).value)) : setType(matrix, b->CreateAnd(type(matrix), constant(~mask, 8*sizeof(likely_type)).value)); }

    Value *depth(Value *matrix) const { return get(matrix, likely_type_depth); }
    void setDepth(Value *matrix, int depth) const { set(matrix, depth, likely_type_depth); }
    Value *signed_(Value *matrix) const { return get(matrix, likely_type_signed); }
    void setSigned(Value *matrix, bool signed_) const { setBit(matrix, signed_, likely_type_signed); }
    Value *floating(Value *matrix) const { return get(matrix, likely_type_floating); }
    void setFloating(Value *matrix, bool floating) const { setBit(matrix, floating, likely_type_floating); }
    Value *parallel(Value *matrix) const { return get(matrix, likely_type_parallel); }
    void setParallel(Value *matrix, bool parallel) const { setBit(matrix, parallel, likely_type_parallel); }
    Value *heterogeneous(Value *matrix) const { return get(matrix, likely_type_heterogeneous); }
    void setHeterogeneous(Value *matrix, bool heterogeneous) const { setBit(matrix, heterogeneous, likely_type_heterogeneous); }
    Value *multiChannel(Value *matrix) const { return get(matrix, likely_type_multi_channel); }
    void setMultiChannel(Value *matrix, bool multiChannel) const { setBit(matrix, multiChannel, likely_type_multi_channel); }
    Value *multiColumn(Value *matrix) const { return get(matrix, likely_type_multi_column); }
    void setMultiColumn(Value *matrix, bool multiColumn) { setBit(matrix, multiColumn, likely_type_multi_column); }
    Value *multiRow(Value *matrix) const { return get(matrix, likely_type_multi_row); }
    void setMultiRow(Value *matrix, bool multiRow) const { setBit(matrix, multiRow, likely_type_multi_row); }
    Value *multiFrame(Value *matrix) const { return get(matrix, likely_type_multi_frame); }
    void multiFrame(Value *matrix, bool multiFrame) const { setBit(matrix, multiFrame, likely_type_multi_frame); }
    Value *saturation(Value *matrix) const { return get(matrix, likely_type_saturation); }
    void setSaturation(Value *matrix, bool saturation) const { setBit(matrix, saturation, likely_type_saturation); }
    Value *reserved(Value *matrix) const { return get(matrix, likely_type_reserved); }
    void setReserved(Value *matrix, int reserved) const { set(matrix, reserved, likely_type_reserved); }

    Value *elements(Value *matrix) const { return b->CreateMul(b->CreateMul(b->CreateMul(channels(matrix), columns(matrix)), rows(matrix)), frames(matrix)); }
    Value *bytes(Value *matrix) const { return b->CreateMul(b->CreateUDiv(b->CreateCast(Instruction::ZExt, depth(matrix), NativeIntegerType), constant(8)), elements(matrix)); }

    Value *columnStep(Value *matrix) const { Value *columnStep = channels(matrix); columnStep->setName("cStep"); return columnStep; }
    Value *rowStep(Value *matrix) const { return b->CreateMul(columns(matrix), columnStep(matrix), "rStep"); }
    Value *frameStep(Value *matrix) const { return b->CreateMul(rows(matrix), rowStep(matrix), "tStep"); }

    Value *index(const TypedValue &matrix, Value *c) const
    {
        if (likely_multi_channel(matrix)) return c;
        else                              return zero();
    }

    Value *index(const TypedValue &matrix, Value *c, Value *x) const
    {
        Value *remainder = index(matrix, c);
        if (likely_multi_column(matrix)) return b->CreateAdd(b->CreateMul(x, columnStep(matrix)), remainder);
        else                             return remainder;
    }

    Value *index(const TypedValue &matrix, Value *c, Value *x, Value *y) const
    {
        Value *remainder = index(matrix, c, x);
        if (likely_multi_row(matrix)) return b->CreateAdd(b->CreateMul(y, rowStep(matrix)), remainder);
        else                          return remainder;
    }

    Value *index(const TypedValue &matrix, Value *c, Value *x, Value *y, Value *t) const
    {
        Value *remainder = index(matrix, c, x, y);
        if (likely_multi_frame(matrix)) return b->CreateAdd(b->CreateMul(t, frameStep(matrix)), remainder);
        else                            return remainder;
    }

    void deindex(const TypedValue &matrix, Value *i, Value **c) const
    {
        (void) matrix;
        *c = i;
    }

    void deindex(const TypedValue &matrix, Value *i, Value **c, Value **x) const
    {

        Value *step = columnStep(matrix);
        Value *remainder = b->CreateURem(i, step, "xRem");
        *x = b->CreateExactUDiv(b->CreateSub(i, remainder), step, "x");
        deindex(matrix, remainder, c);
    }

    void deindex(const TypedValue &matrix, Value *i, Value **c, Value **x, Value **y) const
    {
        Value *step = rowStep(matrix);
        Value *remainder = b->CreateURem(i, step, "yRem");
        *y = b->CreateExactUDiv(b->CreateSub(i, remainder), step, "y");
        deindex(matrix, remainder, c, x);
    }

    void deindex(const TypedValue &matrix, Value *i, Value **c, Value **x, Value **y, Value **t) const
    {
        Value *step = frameStep(matrix);
        Value *remainder = b->CreateURem(i, step, "tRem");
        *t = b->CreateExactUDiv(b->CreateSub(i, remainder), step, "t");
        deindex(matrix, remainder, c, x, y);
    }

    TypedValue GEP(const TypedValue &matrix, Value *i)
    {
        return TypedValue(b->CreateGEP(data(matrix), i), matrix.type);
    }

    TypedValue load(const TypedValue &matrix, Value *i)
    {
        LoadInst *load = b->CreateLoad(GEP(matrix, i));
        load->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return TypedValue(load, matrix.type);
    }

    TypedValue store(const TypedValue &matrix, Value *i, const TypedValue &value)
    {
        StoreInst *store = b->CreateStore(value, GEP(matrix, i));
        store->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return TypedValue(store, matrix.type);
    }

    TypedValue cast(const TypedValue &x, likely_type type) const
    {
        if ((x.type & likely_type_mask) == (type & likely_type_mask))
            return x;
        Type *dstType = ty(type);
        return TypedValue(b->CreateCast(CastInst::getCastOpcode(x, likely_signed(x.type), dstType, likely_signed(type)), x, dstType), type);
    }

    static likely_type validFloatType(likely_type type)
    {
        likely_set_floating(&type, true);
        likely_set_signed(&type, true);
        likely_set_depth(&type, likely_depth(type) > 32 ? 64 : 32);
        return type;
    }

    Loop beginLoop(BasicBlock *entry, Value *start, Value *stop)
    {
        Loop loop;
        loop.stop = stop;
        loop.body = BasicBlock::Create(getGlobalContext(), "loop_body", f);

        // Create self-referencing loop node
        vector<Value*> metadata;
        MDNode *tmp = MDNode::getTemporary(getGlobalContext(), metadata);
        metadata.push_back(tmp);
        loop.node = MDNode::get(getGlobalContext(), metadata);
        tmp->replaceAllUsesWith(loop.node);
        MDNode::deleteTemporary(tmp);

        // Loops assume there is at least one iteration
        b->CreateBr(loop.body);
        b->SetInsertPoint(loop.body);

        loop.i = b->CreatePHI(NativeIntegerType, 2, "i");
        loop.i->addIncoming(start, entry);

        loops.push(loop);
        return loop;
    }

    void endLoop()
    {
        const Loop &loop = loops.top();
        Value *increment = b->CreateAdd(loop.i, one(), "loop_increment");
        BasicBlock *loopLatch = BasicBlock::Create(getGlobalContext(), "loop_latch", f);
        b->CreateBr(loopLatch);
        b->SetInsertPoint(loopLatch);
        BasicBlock *loopExit = BasicBlock::Create(getGlobalContext(), "loop_exit", f);
        BranchInst *latch = b->CreateCondBr(b->CreateICmpEQ(increment, loop.stop, "loop_test"), loopExit, loop.body);
        latch->setMetadata("llvm.loop", loop.node);
        loop.i->addIncoming(increment, loopLatch);
        b->SetInsertPoint(loopExit);
        loops.pop();
    }

    MDNode *getCurrentNode() const { return loops.empty() ? NULL : loops.top().node; }

    template <typename T>
    inline static vector<T> toVector(T value) { vector<T> vector; vector.push_back(value); return vector; }

    static Type *ty(likely_type type, bool pointer = false)
    {
        const int bits = likely_depth(type);
        const bool floating = likely_floating(type);
        if (floating) {
            if      (bits == 16) return pointer ? Type::getHalfPtrTy(getGlobalContext())   : Type::getHalfTy(getGlobalContext());
            else if (bits == 32) return pointer ? Type::getFloatPtrTy(getGlobalContext())  : Type::getFloatTy(getGlobalContext());
            else if (bits == 64) return pointer ? Type::getDoublePtrTy(getGlobalContext()) : Type::getDoubleTy(getGlobalContext());
        } else {
            if      (bits == 1)  return pointer ? Type::getInt1PtrTy(getGlobalContext())  : (Type*)Type::getInt1Ty(getGlobalContext());
            else if (bits == 8)  return pointer ? Type::getInt8PtrTy(getGlobalContext())  : (Type*)Type::getInt8Ty(getGlobalContext());
            else if (bits == 16) return pointer ? Type::getInt16PtrTy(getGlobalContext()) : (Type*)Type::getInt16Ty(getGlobalContext());
            else if (bits == 32) return pointer ? Type::getInt32PtrTy(getGlobalContext()) : (Type*)Type::getInt32Ty(getGlobalContext());
            else if (bits == 64) return pointer ? Type::getInt64PtrTy(getGlobalContext()) : (Type*)Type::getInt64Ty(getGlobalContext());
        }
        likely_assert(false, "MatrixBuilder::ty invalid matrix bits: %d and floating: %d", bits, floating);
        return NULL;
    }
};

struct SExp
{
    string op;
    vector<SExp> sexps;
    SExp() = default;
    SExp(const string &source)
    {
        likely_assert(!source.empty(), "empty expression");
        if ((source[0] == '(') && (source[source.size()-1] == ')')) {
            size_t start = 1, depth = 0;
            for (size_t i=1; i<source.size()-1; i++) {
                if (source[i] == '(') {
                    depth++;
                } else if (source[i] == ')') {
                    likely_assert(depth > 0, "unexpected ')' in: %s", source.c_str());
                    depth--;
                } else if ((source[i] == ' ') && (depth == 0)) {
                    if (start != i)
                        sexps.push_back(source.substr(start, i-start));
                    start = i + 1;
                }
            }
            likely_assert(depth == 0, "missing ')' in :%s", source.c_str());
            if (start != source.size()-1)
                sexps.push_back(source.substr(start, source.size()-1-start));
            likely_assert(sexps.front().sexps.empty(), "unexpected operand: %s", source.c_str());
            op = sexps.front().op;
            sexps.erase(sexps.begin());
        } else {
            likely_assert(source.find(' ') == string::npos, "unexpected ' ' in: %s", source.c_str());
            op = source;
        }
    }
};

struct KernelInfo
{
    vector<TypedValue> srcs;
    TypedValue i, c, x, y, t;
    likely_type dims;
    KernelInfo(const KernelBuilder &kernel, const vector<TypedValue> &srcs_, const TypedValue &dst, Value *i_)
        : srcs(srcs_), i(i_, likely_type_native)
    {
        c.type = x.type = y.type = t.type = i.type;
        kernel.deindex(dst, i, &c.value, &x.value, &y.value, &t.value);
        dims = likely_type_null;
        for (const TypedValue &src : srcs)
            dims |= (src.type & likely_type_multi_dimension);
    }
};

struct Operation
{
    static map<string, const Operation*> operations;
    static void add(const Operation *operation)
    {
        assert(operations.find(operation->name()) == operations.end());
        operations.insert(pair<string, const Operation*>(operation->name(), operation));
    }

    virtual ~Operation() {}
    virtual string name() const = 0;
    virtual TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const = 0;
};
map<string, const Operation*> Operation::operations;

template <class T>
struct RegisterOperation
{
    RegisterOperation()
    {
        Operation::add(new T());
    }
};
#define LIKELY_REGISTER(OPERATION) static struct RegisterOperation<OPERATION> Register##OPERATION;

class NullaryOperation : public Operation
{
    TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.empty());
        return callNullary(kernel, info);
    }
    virtual TypedValue callNullary(KernelBuilder &kernel, const KernelInfo &info) const = 0;
};

class IndexOperation : public NullaryOperation
{
    TypedValue callNullary(KernelBuilder &kernel, const KernelInfo &info) const
    {
        (void) kernel;
        return callIndex(info);
    }
    virtual TypedValue callIndex(const KernelInfo &info) const = 0;
};

class iOperation : public IndexOperation
{
    string name() const { return "i"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.i;
    }
};
LIKELY_REGISTER(iOperation)

class cOperation : public IndexOperation
{
    string name() const { return "c"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.c;
    }
};
LIKELY_REGISTER(cOperation)

class xOperation : public IndexOperation
{
    string name() const { return "x"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.x;
    }
};
LIKELY_REGISTER(xOperation)

class yOperation : public IndexOperation
{
    string name() const { return "y"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.y;
    }
};
LIKELY_REGISTER(yOperation)

class tOperation : public IndexOperation
{
    string name() const { return "t"; }
    TypedValue callIndex(const KernelInfo &info) const
    {
        return info.t;
    }
};
LIKELY_REGISTER(tOperation)

class UnaryOperation : public Operation
{
    TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.size() == 1);
        return callUnary(kernel, info, args[0]);
    }
    virtual TypedValue callUnary(KernelBuilder &kernel, const KernelInfo &info, TypedValue arg) const = 0;
};

class UnaryMathOperation : public UnaryOperation
{
    TypedValue callUnary(KernelBuilder &kernel, const KernelInfo &info, TypedValue x) const
    {
        (void) info;
        x = kernel.cast(x, KernelBuilder::validFloatType(x.type));
        vector<Type*> args;
        args.push_back(x.value->getType());
        return TypedValue(kernel.b->CreateCall(Intrinsic::getDeclaration(kernel.m, id(), args), x), x.type);
    }
    virtual Intrinsic::ID id() const = 0;
};

#define LIKELY_REGISTER_UNARY_MATH(OPERATION)                 \
class OPERATION##Operation : public UnaryMathOperation        \
{                                                             \
    string name() const { return #OPERATION; }                \
    Intrinsic::ID id() const { return Intrinsic::OPERATION; } \
};                                                            \
LIKELY_REGISTER(OPERATION##Operation)                         \

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
    TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.size() == 2);
        return callBinary(kernel, info, args[0], args[1]);
    }
    virtual TypedValue callBinary(KernelBuilder &kernel, const KernelInfo &info, TypedValue arg1, TypedValue arg2) const = 0;
};

class castOperation : public BinaryOperation
{
    string name() const { return "cast"; }
    TypedValue callBinary(KernelBuilder &kernel, const KernelInfo &info, TypedValue x, TypedValue type) const
    {
        (void) info;
        return kernel.cast(x, LLVM_VALUE_TO_INT(type.value));
    }
};
LIKELY_REGISTER(castOperation)

class thresholdOperation : public BinaryOperation
{
    string name() const { return "threshold"; }
    TypedValue callBinary(KernelBuilder &kernel, const KernelInfo &info, TypedValue x, TypedValue t) const
    {
         (void) info;
         likely_type type = likely_type_from_types(x, t);
         x = kernel.cast(x, type);
         t = kernel.cast(t, type);
         Value *comp = likely_floating(type) ? kernel.b->CreateFCmpOLE(x, t) : (likely_signed(type) ? kernel.b->CreateICmpSLE(x, t) : kernel.b->CreateICmpULE(x, t));
         TypedValue high = kernel.constant(1.0, type);
         TypedValue low = kernel.constant(0.0, type);
         return TypedValue(kernel.b->CreateSelect(comp, low, high), type);
    }
};
LIKELY_REGISTER(thresholdOperation)

class ArithmeticOperation : public BinaryOperation
{
    TypedValue callBinary(KernelBuilder &kernel, const KernelInfo &info, TypedValue arg1, TypedValue arg2) const
    {
        (void) info;
        likely_type type = likely_type_from_types(arg1, arg2);
        return callArithmetic(kernel.b, kernel.cast(arg1, type), kernel.cast(arg2, type), type);
    }
    virtual TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const = 0;
};

class addOperation : public ArithmeticOperation
{
    string name() const { return "+"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(b->CreateFAdd(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                if (likely_signed(type)) {
                    const int depth = likely_depth(type);
                    Value *result = b->CreateAdd(lhs, rhs);
                    Value *overflowResult = b->CreateAdd(b->CreateLShr(lhs, depth-1), KernelBuilder::intMax(depth));
                    Value *overflowCondition = b->CreateICmpSGE(b->CreateOr(b->CreateXor(lhs.value, rhs.value), b->CreateNot(b->CreateXor(rhs, result))), KernelBuilder::zero(depth));
                    return TypedValue(b->CreateSelect(overflowCondition, overflowResult, result), type);
                } else {
                    Value *result = b->CreateAdd(lhs, rhs);
                    Value *overflow = b->CreateNeg(b->CreateZExt(b->CreateICmpULT(result, lhs),
                                                   Type::getIntNTy(getGlobalContext(), likely_depth(type))));
                    return TypedValue(b->CreateOr(result, overflow), type);
                }
            } else {
                return TypedValue(b->CreateAdd(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER(addOperation)

class subtractOperation : public ArithmeticOperation
{
    string name() const { return "-"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(b->CreateFSub(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                if (likely_signed(type)) {
                    const int depth = likely_depth(type);
                    Value *result = b->CreateSub(lhs, rhs);
                    Value *overflowResult = b->CreateAdd(b->CreateLShr(lhs, depth-1), KernelBuilder::intMax(depth));
                    Value *overflowCondition = b->CreateICmpSLT(b->CreateAnd(b->CreateXor(lhs.value, rhs.value), b->CreateXor(lhs, result)), KernelBuilder::zero(depth));
                    return TypedValue(b->CreateSelect(overflowCondition, overflowResult, result), type);
                } else {
                    Value *result = b->CreateSub(lhs, rhs);
                    Value *overflow = b->CreateNeg(b->CreateZExt(b->CreateICmpULE(result, lhs),
                                                   Type::getIntNTy(getGlobalContext(), likely_depth(type))));
                    return TypedValue(b->CreateAnd(result, overflow), type);
                }
            } else {
                return TypedValue(b->CreateSub(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER(subtractOperation)

class multiplyOperation : public ArithmeticOperation
{
    string name() const { return "*"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(b->CreateFMul(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                const int depth = likely_depth(type);
                Type *originalType = Type::getIntNTy(getGlobalContext(), depth);
                Type *extendedType = Type::getIntNTy(getGlobalContext(), 2*depth);
                Value *result = b->CreateMul(b->CreateZExt(lhs, extendedType),
                                             b->CreateZExt(rhs, extendedType));
                Value *lo = b->CreateTrunc(result, originalType);

                if (likely_signed(type)) {
                    Value *hi = b->CreateTrunc(b->CreateAShr(result, depth), originalType);
                    Value *overflowResult = b->CreateAdd(b->CreateLShr(b->CreateXor(lhs.value, rhs.value), depth-1), KernelBuilder::intMax(depth));
                    Value *overflowCondition = b->CreateICmpNE(hi, b->CreateAShr(lo, depth-1));
                    return TypedValue(b->CreateSelect(overflowCondition, overflowResult, b->CreateTrunc(result, lhs.value->getType())), type);
                } else {
                    Value *hi = b->CreateTrunc(b->CreateLShr(result, depth), originalType);
                    Value *overflow = b->CreateNeg(b->CreateZExt(b->CreateICmpNE(hi, KernelBuilder::zero(depth)), originalType));
                    return TypedValue(b->CreateOr(lo, overflow), type);
                }
            } else {
                return TypedValue(b->CreateMul(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER(multiplyOperation)

class divideOperation : public ArithmeticOperation
{
    string name() const { return "/"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue n, TypedValue d, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(b->CreateFDiv(n, d), type);
        } else {
            if (likely_signed(type)) {
                if (likely_saturation(type)) {
                    const int depth = likely_depth(type);
                    Value *safe_i = b->CreateAdd(n, b->CreateZExt(b->CreateICmpNE(b->CreateOr(b->CreateAdd(d, KernelBuilder::constant(1, depth)), b->CreateAdd(n, KernelBuilder::intMin(depth))), KernelBuilder::zero(depth)), n.value->getType()));
                    return TypedValue(b->CreateSDiv(safe_i, d), type);
                } else {
                    return TypedValue(b->CreateSDiv(n, d), type);
                }
            } else {
                return TypedValue(b->CreateUDiv(n, d), type);
            }
        }
    }
};
LIKELY_REGISTER(divideOperation)

class ltOperation : public ArithmeticOperation
{
    string name() const { return "<"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOLT(lhs, rhs) : (likely_signed(type) ? b->CreateICmpSLT(lhs, rhs) : b->CreateICmpULT(lhs, rhs));
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(ltOperation)

class leOperation : public ArithmeticOperation
{
    string name() const { return "<="; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOLE(lhs, rhs) : (likely_signed(type) ? b->CreateICmpSLE(lhs, rhs) : b->CreateICmpULE(lhs, rhs));
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(leOperation)

class gtOperation : public ArithmeticOperation
{
    string name() const { return ">"; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOGT(lhs, rhs) : (likely_signed(type) ? b->CreateICmpSGT(lhs, rhs) : b->CreateICmpUGT(lhs, rhs));
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(gtOperation)

class geOperation : public ArithmeticOperation
{
    string name() const { return ">="; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOGE(lhs, rhs) : (likely_signed(type) ? b->CreateICmpSGE(lhs, rhs) : b->CreateICmpUGE(lhs, rhs));
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(geOperation)

class eqOperation : public ArithmeticOperation
{
    string name() const { return "=="; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpOEQ(lhs, rhs) : b->CreateICmpEQ(lhs, rhs);
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(eqOperation)

class neOperation : public ArithmeticOperation
{
    string name() const { return "!="; }
    TypedValue callArithmetic(IRBuilder<> *b, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        Value *comp = likely_floating(type) ? b->CreateFCmpONE(lhs, rhs) : b->CreateICmpNE(lhs, rhs);
        return TypedValue(comp, type);
    }
};
LIKELY_REGISTER(neOperation)

class BinaryMathOperation : public BinaryOperation
{
    TypedValue callBinary(KernelBuilder &kernel, const KernelInfo &info, TypedValue x, TypedValue n) const
    {
        (void) info;
        const likely_type type = nIsInteger() ? x.type : likely_type_from_types(x, n);
        x = kernel.cast(x, KernelBuilder::validFloatType(type));
        n = kernel.cast(n, nIsInteger() ? likely_type_i32 : x.type);
        vector<Type*> args;
        args.push_back(x.value->getType());
        return TypedValue(kernel.b->CreateCall2(Intrinsic::getDeclaration(kernel.m, id(), args), x, n), x.type);
    }
    virtual Intrinsic::ID id() const = 0;
    virtual bool nIsInteger() const { return false; }
};

class powiOperation : public BinaryMathOperation
{
    string name() const { return "powi"; }
    Intrinsic::ID id() const { return Intrinsic::powi; }
    bool nIsInteger() const { return true; }
};
LIKELY_REGISTER(powiOperation)

class powOperation : public BinaryMathOperation
{
    string name() const { return "pow"; }
    Intrinsic::ID id() const { return Intrinsic::pow; }
};
LIKELY_REGISTER(powOperation)

class copysignOperation : public BinaryMathOperation
{
    string name() const { return "copysign"; }
    Intrinsic::ID id() const { return Intrinsic::copysign; }
};
LIKELY_REGISTER(copysignOperation)

class TernaryOperation : public Operation
{
    TypedValue call(KernelBuilder &kernel, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.size() == 3);
        return callTernary(kernel, info, args[0], args[1], args[2]);
    }
    virtual TypedValue callTernary(KernelBuilder &kernel, const KernelInfo &info, TypedValue arg1, TypedValue arg2, TypedValue arg3) const = 0;
};

class fmaOperation : public TernaryOperation
{
    string name() const { return "fma"; }
    TypedValue callTernary(KernelBuilder &kernel, const KernelInfo &info, TypedValue a, TypedValue x, TypedValue c) const
    {
        (void) info;
        const likely_type type = likely_type_from_types(likely_type_from_types(a, x), c);
        x = kernel.cast(x, KernelBuilder::validFloatType(type));
        a = kernel.cast(a, x.type);
        c = kernel.cast(c, x.type);
        vector<Type*> args;
        args.push_back(x.value->getType());
        return TypedValue(kernel.b->CreateCall3(Intrinsic::getDeclaration(kernel.m, Intrinsic::fma, args), x, a, c), x.type);
    }
};
LIKELY_REGISTER(fmaOperation)

struct LikelyKernelOptimizationPass : public FunctionPass
{
    static char ID;
    LikelyKernelOptimizationPass() : FunctionPass(ID) {}

    struct MatrixInfo
    {
        Value *channels = NULL,
              *columns  = NULL,
              *rows     = NULL,
              *frames   = NULL,
              *columnStep = NULL,
              *rowStep    = NULL,
              *frameStep  = NULL;
    };

    bool runOnFunction(Function &F)
    {
        DEBUG(dbgs() << "LKO: " << F.getName() << "\n");

        vector<Argument*> matricies;
        Argument *start = NULL, *stop = NULL;

        Function::arg_iterator args = F.arg_begin();
        while ((args != F.arg_end()) && (args->getType() == PointerType::getUnqual(TheMatrixStruct)))
            matricies.push_back(args++);
        if ((args != F.arg_end()) && (args->getType() == NativeIntegerType))
            start = args++;
        if ((args != F.arg_end()) && (args->getType() == NativeIntegerType))
            stop = args++;
        if (matricies.empty() || !start || !stop || (args != F.arg_end()))
            return false;
        DEBUG(dbgs() << "LKO: found a kernel with " << matricies.size() << " matricies\n");

        vector<MatrixInfo> matrixInfo(matricies.size());
        (void) matrixInfo;

        return false;
    }
};
char LikelyKernelOptimizationPass::ID = 0;
static RegisterPass<LikelyKernelOptimizationPass> RegisterLikelyKernelOptimizationPass("likely", "Likely Kernel Optimization Pass", false, false);

// Control parallel execution
static vector<mutex*> workers;
static mutex workersActive;
static atomic_uint workersRemaining(0);
static void *currentThunk = NULL;
static likely_arity thunkArity = 0;
static likely_size thunkSize = 0;
static likely_const_mat thunkMatricies[LIKELY_NUM_ARITIES+1];

static void executeWorker(int id, int numWorkers)
{
    // There are hardware_concurrency-1 helper threads and the main thread with id = 0
    const likely_size step = (thunkSize+numWorkers-1)/numWorkers;
    const likely_size start = id * step;
    const likely_size stop = std::min((id+1)*step, thunkSize);
    if (start >= stop) return;

    // Final three parameters are: dst, start, stop
    typedef void (*likely_kernel_0)(likely_mat, likely_size, likely_size);
    typedef void (*likely_kernel_1)(likely_const_mat, likely_mat, likely_size, likely_size);
    typedef void (*likely_kernel_2)(likely_const_mat, likely_const_mat, likely_mat, likely_size, likely_size);
    typedef void (*likely_kernel_3)(likely_const_mat, likely_const_mat, likely_const_mat, likely_mat, likely_size, likely_size);

    switch (thunkArity) {
      case 0: reinterpret_cast<likely_kernel_0>(currentThunk)((likely_mat)thunkMatricies[0], start, stop); break;
      case 1: reinterpret_cast<likely_kernel_1>(currentThunk)(thunkMatricies[0], (likely_mat)thunkMatricies[1], start, stop); break;
      case 2: reinterpret_cast<likely_kernel_2>(currentThunk)(thunkMatricies[0], thunkMatricies[1], (likely_mat)thunkMatricies[2], start, stop); break;
      case 3: reinterpret_cast<likely_kernel_3>(currentThunk)(thunkMatricies[0], thunkMatricies[1], thunkMatricies[2], (likely_mat)thunkMatricies[3], start, stop); break;
      default: likely_assert(false, "executeWorker invalid arity: %d", thunkArity);
    }
}

static void workerThread(int id, int numWorkers)
{
    while (true) {
        workers[id]->lock();
        executeWorker(id, numWorkers);
        if (--workersRemaining == 0)
            workersActive.unlock();
    }
}

class FunctionBuilder
{
    string name;
    SExp sexp;
    vector<likely_type> types;
    Module *module;
    ExecutionEngine *executionEngine;
    TargetMachine *targetMachine;

public:
    FunctionBuilder(const string &name_, const SExp &sexp_, const vector<likely_type> &types_)
        : name(name_), sexp(sexp_), types(types_)
    {
        if (TheMatrixStruct == NULL) {
            assert(sizeof(likely_size) == sizeof(void*));
            InitializeNativeTarget();
            InitializeNativeTargetAsmPrinter();
            InitializeNativeTargetAsmParser();
            initializeScalarOpts(*PassRegistry::getPassRegistry());

            likely_set_depth(&likely_type_native, sizeof(likely_size)*8);
            NativeIntegerType = Type::getIntNTy(getGlobalContext(), likely_depth(likely_type_native));
            TheMatrixStruct = StructType::create("likely_matrix",
                                                 Type::getInt8PtrTy(getGlobalContext()), // data
                                                 PointerType::getUnqual(StructType::create(getGlobalContext(), "likely_matrix_private")), // d_ptr
                                                 NativeIntegerType,                      // channels
                                                 NativeIntegerType,                      // columns
                                                 NativeIntegerType,                      // rows
                                                 NativeIntegerType,                      // frames
                                                 Type::getInt32Ty(getGlobalContext()),   // type
                                                 NULL);
        }

        module = new Module(name, getGlobalContext());
        likely_assert(module != NULL, "failed to create LLVM Module");
        module->setTargetTriple(sys::getProcessTriple());

        string error;
        EngineBuilder engineBuilder(module);
        engineBuilder.setMCPU(sys::getHostCPUName());
        engineBuilder.setEngineKind(EngineKind::JIT);
        engineBuilder.setOptLevel(CodeGenOpt::Aggressive);
        engineBuilder.setErrorStr(&error);
        engineBuilder.setUseMCJIT(true);

        executionEngine = engineBuilder.create();
        likely_assert(executionEngine != NULL, "failed to create LLVM ExecutionEngine with error: %s", error.c_str());

        targetMachine = engineBuilder.selectTarget();
        likely_assert(targetMachine != NULL, "failed to create LLVM TargetMachine with error: %s", error.c_str());
    }

    ~FunctionBuilder()
    {
        delete targetMachine;
        delete executionEngine;
        delete module;
    }

    void *getPointerToFunction()
    {
        Function *function = module->getFunction(name);
        if (function != NULL)
            return executionEngine->getPointerToFunction(function);
        function = getFunction(name, module, types.size(), PointerType::getUnqual(TheMatrixStruct));

        Function *thunk;
        likely_type dstType;
        {
            thunk = getFunction(name+"_thunk", module, types.size(), Type::getVoidTy(getGlobalContext()), PointerType::getUnqual(TheMatrixStruct), NativeIntegerType, NativeIntegerType);
            vector<TypedValue> srcs;
            getValues(thunk, types, srcs);
            TypedValue stop = srcs.back(); srcs.pop_back();
            stop.value->setName("stop");
            stop.type = likely_type_native;
            TypedValue start = srcs.back(); srcs.pop_back();
            start.value->setName("start");
            start.type = likely_type_native;
            TypedValue dst = srcs.back(); srcs.pop_back();
            dst.value->setName("dst");

            BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", thunk);
            IRBuilder<> builder(entry);
            KernelBuilder kernel(module, &builder, thunk);
            Value *i = kernel.beginLoop(entry, start, stop).i;
            KernelInfo info(kernel, srcs, dst, i);

            TypedValue result = generateKernelRecursive(kernel, info, sexp);

            dstType = result.type;
            kernel.store(TypedValue(dst.value, dstType), i, result);
            kernel.endLoop();
            builder.CreateRetVoid();

            FunctionPassManager functionPassManager(module);
            functionPassManager.add(createVerifierPass(PrintMessageAction));
            functionPassManager.add(new LikelyKernelOptimizationPass());
            targetMachine->addAnalysisPasses(functionPassManager);
            functionPassManager.add(new TargetLibraryInfo(Triple(module->getTargetTriple())));
            functionPassManager.add(new DataLayout(module));
            functionPassManager.add(createBasicAliasAnalysisPass());
            functionPassManager.add(createLICMPass());
            functionPassManager.add(createLoopVectorizePass());
            functionPassManager.add(createInstructionCombiningPass());
            functionPassManager.add(createEarlyCSEPass());
            functionPassManager.add(createCFGSimplificationPass());
            functionPassManager.doInitialization();
//            DebugFlag = true;
            functionPassManager.run(*thunk);
        }

        static FunctionType* LikelyNewSignature = NULL;
        if (LikelyNewSignature == NULL) {
            Type *newReturn = PointerType::getUnqual(TheMatrixStruct);
            vector<Type*> newParameters;
            newParameters.push_back(Type::getInt32Ty(getGlobalContext())); // type
            newParameters.push_back(NativeIntegerType); // channels
            newParameters.push_back(NativeIntegerType); // columns
            newParameters.push_back(NativeIntegerType); // rows
            newParameters.push_back(NativeIntegerType); // frames
            newParameters.push_back(Type::getInt8PtrTy(getGlobalContext())); // data
            newParameters.push_back(Type::getInt8Ty(getGlobalContext())); // copy
            LikelyNewSignature = FunctionType::get(newReturn, newParameters, false);
        }
        Function *likelyNew = Function::Create(LikelyNewSignature, GlobalValue::ExternalLinkage, "likely_new", module);
        likelyNew->setCallingConv(CallingConv::C);
        likelyNew->setDoesNotAlias(0);
        likelyNew->setDoesNotAlias(6);
        likelyNew->setDoesNotCapture(6);

        vector<TypedValue> srcs;
        getValues(function, types, srcs);
        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);
        KernelBuilder kernel(module, &builder, function);

        Value *dstChannels = NULL, *dstColumns = NULL, *dstRows = NULL, *dstFrames = NULL;
        for (size_t i=0; i<types.size(); i++) {
            if (!dstChannels && likely_multi_channel(types[i])) dstChannels = kernel.channels(srcs[i]);
            if (!dstColumns  && likely_multi_column(types[i]))  dstColumns  = kernel.columns(srcs[i]);
            if (!dstRows     && likely_multi_row(types[i]))     dstRows     = kernel.rows(srcs[i]);
            if (!dstFrames   && likely_multi_frame(types[i]))   dstFrames   = kernel.frames(srcs[i]);
        }
        if (!dstChannels) { dstChannels = kernel.constant(1); assert(!likely_multi_channel(dstType)); }
        if (!dstColumns)  { dstColumns  = kernel.constant(1); assert(!likely_multi_column(dstType));  }
        if (!dstRows)     { dstRows     = kernel.constant(1); assert(!likely_multi_row(dstType));     }
        if (!dstFrames)   { dstFrames   = kernel.constant(1); assert(!likely_multi_frame(dstType));   }

        std::vector<Value*> likelyNewArgs;
        likelyNewArgs.push_back(kernel.type(dstType));
        likelyNewArgs.push_back(dstChannels);
        likelyNewArgs.push_back(dstColumns);
        likelyNewArgs.push_back(dstRows);
        likelyNewArgs.push_back(dstFrames);
        likelyNewArgs.push_back(ConstantPointerNull::get(Type::getInt8PtrTy(getGlobalContext())));
        likelyNewArgs.push_back(kernel.constant(0, 8));
        Value *dst = builder.CreateCall(likelyNew, likelyNewArgs);

        Value *kernelSize = builder.CreateMul(builder.CreateMul(builder.CreateMul(dstChannels, dstColumns), dstRows), dstFrames);
        if (likely_parallel(types[0])) {
            if (workers.empty()) {
                const int numWorkers = std::max((int)thread::hardware_concurrency(), 1);
                workers.push_back(NULL); // main thread = 0
                for (int i=1; i<numWorkers; i++) {
                    mutex *m = new mutex();
                    m->lock();
                    workers.push_back(m);
                    thread(workerThread, i, numWorkers).detach();
                }
            }

            static FunctionType *likelyForkType = NULL;
            if (likelyForkType == NULL) {
                vector<Type*> likelyForkParameters;
                likelyForkParameters.push_back(thunk->getType());
                likelyForkParameters.push_back(Type::getInt8Ty(getGlobalContext()));
                likelyForkParameters.push_back(NativeIntegerType);
                likelyForkParameters.push_back(PointerType::getUnqual(TheMatrixStruct));
                Type *likelyForkReturn = Type::getVoidTy(getGlobalContext());
                likelyForkType = FunctionType::get(likelyForkReturn, likelyForkParameters, true);
            }
            Function *likelyFork = Function::Create(likelyForkType, GlobalValue::ExternalLinkage, "likely_fork", module);
            likelyFork->setCallingConv(CallingConv::C);
            likelyFork->setDoesNotCapture(4);
            likelyFork->setDoesNotAlias(4);

            vector<Value*> likelyForkArgs;
            likelyForkArgs.push_back(module->getFunction(thunk->getName()));
            likelyForkArgs.push_back(KernelBuilder::constant(types.size(), 8));
            likelyForkArgs.push_back(kernelSize);
            likelyForkArgs.insert(likelyForkArgs.end(), srcs.begin(), srcs.end());
            likelyForkArgs.push_back(dst);
            builder.CreateCall(likelyFork, likelyForkArgs);
        } else {
            vector<Value*> thunkArgs;
            for (const TypedValue &src : srcs)
                thunkArgs.push_back(src.value);
            thunkArgs.push_back(dst);
            thunkArgs.push_back(KernelBuilder::zero());
            thunkArgs.push_back(kernelSize);
            builder.CreateCall(thunk, thunkArgs);
        }

        builder.CreateRet(dst);

        FunctionPassManager functionPassManager(module);
        functionPassManager.add(createVerifierPass(PrintMessageAction));
        functionPassManager.add(createInstructionCombiningPass());
        functionPassManager.run(*function);

//        module->dump();
        executionEngine->finalizeObject();
        return executionEngine->getPointerToFunction(function);
    }

private:
    static Function *getFunction(const string &name, Module *m, likely_arity arity, Type *ret, Type *dst = NULL, Type *start = NULL, Type *stop = NULL)
    {
        PointerType *matrixPointer = PointerType::getUnqual(TheMatrixStruct);
        Function *function;
        switch (arity) {
          case 0: function = cast<Function>(m->getOrInsertFunction(name, ret, dst, start, stop, NULL)); break;
          case 1: function = cast<Function>(m->getOrInsertFunction(name, ret, matrixPointer, dst, start, stop, NULL)); break;
          case 2: function = cast<Function>(m->getOrInsertFunction(name, ret, matrixPointer, matrixPointer, dst, start, stop, NULL)); break;
          case 3: function = cast<Function>(m->getOrInsertFunction(name, ret, matrixPointer, matrixPointer, matrixPointer, dst, start, stop, NULL)); break;
          default: { function = NULL; likely_assert(false, "FunctionBuilder::getFunction invalid arity: %d", arity); }
        }
        function->addFnAttr(Attribute::NoUnwind);
        function->setCallingConv(CallingConv::C);
        if (ret->isPointerTy())
            function->setDoesNotAlias(0);
        size_t num_mats = arity;
        if (dst) num_mats++;
        for (size_t i=0; i<num_mats; i++) {
            function->setDoesNotAlias(i+1);
            function->setDoesNotCapture(i+1);
        }
        return function;
    }

    static void getValues(Function *function, const vector<likely_type> &types, vector<TypedValue> &srcs)
    {
        Function::arg_iterator args = function->arg_begin();
        likely_arity arity = 0;
        while (args != function->arg_end()) {
            Value *src = args++;
            stringstream name; name << "__" << int(arity);
            src->setName(name.str());
            srcs.push_back(TypedValue(src, arity < types.size() ? types[arity] : likely_type_null));
            arity++;
        }
    }

    TypedValue constant(const string &str, bool *ok)
    {
        // Split value from type
        size_t i = 0;
        while ((i < str.size()) && ((str[i] != 'u') || (str[i] != 'i') || (str[i] != 'f')))
            i++;

        // Parse string
        char *p;
        const double value = strtod(str.substr(0, i).c_str(), &p);
        const likely_type type = (  i == str.size()
                                  ? likely_type_from_value(value)
                                  : likely_type_from_string(str.substr(i, str.size()-i).c_str()));

        *ok = ((*p == 0) && (type != likely_type_null));
        return KernelBuilder::constant(value, type);
    }

    TypedValue generateKernelRecursive(KernelBuilder &kernel, const KernelInfo &info, const SExp &expression)
    {
        vector<TypedValue> operands;
        for (const SExp &operand : expression.sexps)
            operands.push_back(generateKernelRecursive(kernel, info, operand));
        const string &op = expression.op;

        map<string,const Operation*>::iterator it = Operation::operations.find(op);
        if (it != Operation::operations.end())
            return it->second->call(kernel, info, operands);

        if (op.substr(0,2) == "__") {
            int index = atoi(op.substr(2, op.size()-2).c_str());
            const TypedValue matrix = info.srcs[index];
            Value *matrix_i;
            if ((matrix.type & likely_type_multi_dimension) == info.dims) {
                // This matrix has the same dimensionality as the output
                matrix_i = info.i;
            } else {
                Value *c, *x, *y, *t;
                kernel.deindex(matrix, info.i, &c, &x, &y, &t);
                matrix_i = kernel.index(matrix, c, x, y, t);
            }
            return TypedValue(kernel.load(matrix, matrix_i), matrix.type);
        } else {
            bool ok;
            TypedValue c = constant(op, &ok);
            likely_assert(ok, "unrecognized literal: %s", op.c_str());
            return c;
        }

        return TypedValue();
    }
};

void *likely_compile(likely_description description, likely_arity n, likely_type type, ...)
{
    vector<likely_type> srcs;
    va_list ap;
    va_start(ap, type);
    for (int i=0; i<n; i++) {
        srcs.push_back(type);
        type = va_arg(ap, likely_type);
    }
    va_end(ap);
    return likely_compile_n(description, n, srcs.data());
}

void *likely_compile_n(likely_description description, likely_arity n, likely_type *types_)
{
    const vector<likely_type> types(types_,types_+n);
    static map<string,FunctionBuilder*> kernels;
    static recursive_mutex makerLock;
    lock_guard<recursive_mutex> lock(makerLock);

    static int index = 0;
    stringstream name; name << "likely_" << index++;

    stringstream uniqueIDStream;
    for (likely_type type : types)
        uniqueIDStream << likely_type_to_string(type) << "_";
    uniqueIDStream << "_" << description;
    const string uniqueID = uniqueIDStream.str();

    map<string,FunctionBuilder*>::const_iterator it = kernels.find(uniqueID);
    if (it == kernels.end()) {
        kernels.insert(pair<string,FunctionBuilder*>(uniqueID, new FunctionBuilder(name.str(), SExp(description), types)));
        it = kernels.find(uniqueID);
    }
    return it->second->getPointerToFunction();
}

void likely_fork(void *thunk, likely_arity arity, likely_size size, likely_const_mat src, ...)
{
    workersActive.lock();

    currentThunk = thunk;
    thunkArity = arity;
    thunkSize = size;

    va_list ap;
    va_start(ap, src);
    for (int i=0; i<arity+1; i++) {
        thunkMatricies[i] = src;
        src = va_arg(ap, likely_const_mat);
    }
    va_end(ap);

    // Skip myself when starting other threads
    workersRemaining = workers.size()-1;
    for (size_t i=1; i<workers.size(); i++)
        workers[i]->unlock();

    executeWorker(0, workers.size());
    while (workersRemaining > 0) {}
}
