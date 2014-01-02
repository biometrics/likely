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
#include <llvm/IR/Constants.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetLibraryInfo.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Vectorize.h>
#include <lua.hpp>
#include <atomic>
#include <condition_variable>
#include <cstdarg>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <stack>
#include <sstream>
#include <thread>

#include "likely/likely_core.h"

using namespace llvm;
using namespace std;

#define LIKELY_NUM_ARITIES 4

#define LLVM_VALUE_IS_INT(VALUE) (llvm::isa<Constant>(VALUE))
#define LLVM_VALUE_TO_INT(VALUE) (llvm::cast<Constant>(VALUE)->getUniqueInteger().getZExtValue())

static likely_type likely_type_native = likely_type_null;
static IntegerType *NativeIntegerType = NULL;
static StructType *TheMatrixStruct = NULL;

typedef struct likely_matrix_private
{
    size_t data_bytes;
    int ref_count;
} likely_matrix_private;

// These symbols needs to be found at run time
extern "C" {
LIKELY_EXPORT void likely_fork(void *thunk, likely_arity arity, likely_size size, likely_const_mat src, ...);
LIKELY_EXPORT likely_mat likely_dispatch(struct VTable *vtable, likely_mat *mats);
}

void likely_assert(bool condition, const char *format, ...)
{
    if (condition) return;
    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "Likely ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\n");
    abort();
}

static int likely_get(likely_type type, likely_type_field mask) { return type & mask; }
static void likely_set(likely_type *type, int i, likely_type_field mask) { *type &= ~mask; *type |= i & mask; }
static bool likely_get_bool(likely_type type, likely_type_field mask) { return (type & mask) != 0; }
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

static likely_data dataPointer(likely_mat m)
{
    return reinterpret_cast<likely_data>(uintptr_t(m+1) + sizeof(likely_matrix_private));
}

likely_mat likely_new(likely_type type, likely_size channels, likely_size columns, likely_size rows, likely_size frames, likely_data data, int8_t copy)
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

    likely_set_multi_channel(&m->type, channels > 1);
    likely_set_multi_column(&m->type, columns > 1);
    likely_set_multi_row(&m->type, rows > 1);
    likely_set_multi_frame(&m->type, frames > 1);

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

likely_mat likely_copy(likely_const_mat m, int8_t clone)
{
    return likely_new(m->type, m->channels, m->columns, m->rows, m->frames, m->data, clone);
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

likely_mat likely_scalar(double value)
{
    likely_mat m = likely_new(likely_type_from_value(value), 1, 1, 1, 1, NULL, 0);
    likely_set_element(m, value, 0, 0, 0, 0);
    return m;
}

double likely_element(likely_const_mat m, likely_size c, likely_size x, likely_size y, likely_size t)
{
    assert(m);
    const likely_size columnStep = m->channels;
    const likely_size rowStep = m->columns * columnStep;
    const likely_size frameStep = m->rows * rowStep;
    const likely_size index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (m->type & likely_type_mask) {
      case likely_type_u8:  return (double)reinterpret_cast< uint8_t*>(m->data)[index];
      case likely_type_u16: return (double)reinterpret_cast<uint16_t*>(m->data)[index];
      case likely_type_u32: return (double)reinterpret_cast<uint32_t*>(m->data)[index];
      case likely_type_u64: return (double)reinterpret_cast<uint64_t*>(m->data)[index];
      case likely_type_i8:  return (double)reinterpret_cast<  int8_t*>(m->data)[index];
      case likely_type_i16: return (double)reinterpret_cast< int16_t*>(m->data)[index];
      case likely_type_i32: return (double)reinterpret_cast< int32_t*>(m->data)[index];
      case likely_type_i64: return (double)reinterpret_cast< int64_t*>(m->data)[index];
      case likely_type_f32: return (double)reinterpret_cast<   float*>(m->data)[index];
      case likely_type_f64: return (double)reinterpret_cast<  double*>(m->data)[index];
      default: assert(false && "likely_element unsupported type");
    }
    return numeric_limits<double>::quiet_NaN();
}

void likely_set_element(likely_mat m, double value, likely_size c, likely_size x, likely_size y, likely_size t)
{
    assert(m);
    const likely_size columnStep = m->channels;
    const likely_size rowStep = m->columns * columnStep;
    const likely_size frameStep = m->rows * rowStep;
    const likely_size index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (m->type & likely_type_mask) {
      case likely_type_u8:  reinterpret_cast< uint8_t*>(m->data)[index] = ( uint8_t)value; break;
      case likely_type_u16: reinterpret_cast<uint16_t*>(m->data)[index] = (uint16_t)value; break;
      case likely_type_u32: reinterpret_cast<uint32_t*>(m->data)[index] = (uint32_t)value; break;
      case likely_type_u64: reinterpret_cast<uint64_t*>(m->data)[index] = (uint64_t)value; break;
      case likely_type_i8:  reinterpret_cast<  int8_t*>(m->data)[index] = (  int8_t)value; break;
      case likely_type_i16: reinterpret_cast< int16_t*>(m->data)[index] = ( int16_t)value; break;
      case likely_type_i32: reinterpret_cast< int32_t*>(m->data)[index] = ( int32_t)value; break;
      case likely_type_i64: reinterpret_cast< int64_t*>(m->data)[index] = ( int64_t)value; break;
      case likely_type_f32: reinterpret_cast<   float*>(m->data)[index] = (   float)value; break;
      case likely_type_f64: reinterpret_cast<  double*>(m->data)[index] = (  double)value; break;
      default: assert(false && "likely_set_element unsupported type");
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
    if (likely_multi_channel(type))  typeStream << "C";
    if (likely_multi_column(type))   typeStream << "X";
    if (likely_multi_row(type))      typeStream << "Y";
    if (likely_multi_frame(type))    typeStream << "T";
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
};

struct ExpressionBuilder : public IRBuilder<>
{
    BasicBlock *entry;
    Module *module;
    Function *function;

    struct Loop {
        BasicBlock *body;
        PHINode *i;
        Value *stop;
        MDNode *node;
    };
    stack<Loop> loops;

    ExpressionBuilder(Module *module, Function *function)
        : IRBuilder<>(getGlobalContext()), module(module), function(function)
    {
        entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        SetInsertPoint(entry);
    }

    static TypedValue constant(double value, likely_type type = likely_type_native)
    {
        const int depth = likely_depth(type);
        if (likely_floating(type)) {
            if (value == 0) value = -0; // IEEE/LLVM optimization quirk
            if      (depth == 64) return TypedValue(ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value), type);
            else if (depth == 32) return TypedValue(ConstantFP::get(Type::getFloatTy(getGlobalContext()), value), type);
            else                  { likely_assert(false, "invalid floating point constant depth: %d", depth); return TypedValue(); }
        } else {
            return TypedValue(Constant::getIntegerValue(Type::getIntNTy(getGlobalContext(), depth), APInt(depth, uint64_t(value))), type);
        }
    }

    static TypedValue zero(int bits = likely_depth(likely_type_native)) { return constant(0, bits); }
    static TypedValue one(int bits = likely_depth(likely_type_native)) { return constant(1, bits); }
    static TypedValue intMax(int bits) { return constant((1 << (bits-1))-1, bits); }
    static TypedValue intMin(int bits) { return constant((1 << (bits-1)), bits); }
    static TypedValue type(likely_type type) { return constant(type, int(sizeof(likely_type)*8)); }

    TypedValue data    (const TypedValue &matrix) { return TypedValue(CreatePointerCast(CreateLoad(CreateStructGEP(matrix, 0), "data"), ty(matrix, true)), matrix.type & likely_type_mask); }
    TypedValue channels(const TypedValue &matrix) { return likely_multi_channel(matrix) ? TypedValue(CreateLoad(CreateStructGEP(matrix, 2), "channels"), likely_type_native) : one(); }
    TypedValue columns (const TypedValue &matrix) { return likely_multi_column (matrix) ? TypedValue(CreateLoad(CreateStructGEP(matrix, 3), "columns" ), likely_type_native) : one(); }
    TypedValue rows    (const TypedValue &matrix) { return likely_multi_row    (matrix) ? TypedValue(CreateLoad(CreateStructGEP(matrix, 4), "rows"    ), likely_type_native) : one(); }
    TypedValue frames  (const TypedValue &matrix) { return likely_multi_frame  (matrix) ? TypedValue(CreateLoad(CreateStructGEP(matrix, 5), "frames"  ), likely_type_native) : one(); }
    TypedValue type    (const TypedValue &matrix) { return TypedValue(CreateLoad(CreateStructGEP(matrix, 6), "type"), likely_type_u32); }

    void setData    (const TypedValue &matrix, const TypedValue &value) { CreateStore(value, CreateStructGEP(matrix, 0)); }
    void setChannels(const TypedValue &matrix, const TypedValue &value) { CreateStore(value, CreateStructGEP(matrix, 2)); }
    void setColumns (const TypedValue &matrix, const TypedValue &value) { CreateStore(value, CreateStructGEP(matrix, 3)); }
    void setRows    (const TypedValue &matrix, const TypedValue &value) { CreateStore(value, CreateStructGEP(matrix, 4)); }
    void setFrames  (const TypedValue &matrix, const TypedValue &value) { CreateStore(value, CreateStructGEP(matrix, 5)); }
    void setType    (const TypedValue &matrix, const TypedValue &value) { CreateStore(value, CreateStructGEP(matrix, 6)); }

    TypedValue get(const TypedValue &matrix, likely_type mask)
    {
        return TypedValue(CreateAnd(type(matrix),
                                    constant(mask, 8*sizeof(likely_type)).value), likely_type_u32);
    }

    void set(const TypedValue &matrix, likely_type mask, int value)
    {
        setType(matrix, TypedValue(CreateOr(CreateAnd(type(matrix),
                                                      constant(~mask, 8*sizeof(likely_type)).value),
                                            CreateAnd(constant(value, 8*sizeof(likely_type)),
                                                      constant( mask, 8*sizeof(likely_type)).value)), likely_type_u32));
    }

    void setBit(const TypedValue &matrix, likely_type mask, bool on)
    {
        Value *value;
        if (on) value = CreateOr (type(matrix), constant( mask, 8*sizeof(likely_type)).value);
        else    value = CreateAnd(type(matrix), constant(~mask, 8*sizeof(likely_type)).value);
        setType(matrix, TypedValue(value, likely_type_u32));
    }

    TypedValue depth(const TypedValue &matrix)     { return get(matrix, likely_type_depth); }
    void    setDepth(const TypedValue &matrix, int depth) { set(matrix, likely_type_depth, depth); }
    TypedValue signed_(const TypedValue &matrix)          { return get(matrix, likely_type_signed); }
    void    setSigned (const TypedValue &matrix, bool signed_) { setBit(matrix, likely_type_signed, signed_); }
    TypedValue floating(const TypedValue &matrix)            { return get(matrix, likely_type_floating); }
    void    setFloating(const TypedValue &matrix, bool floating) { setBit(matrix, likely_type_floating, floating); }
    TypedValue parallel(const TypedValue &matrix)            { return get(matrix, likely_type_parallel); }
    void    setParallel(const TypedValue &matrix, bool parallel) { setBit(matrix, likely_type_parallel, parallel); }
    TypedValue heterogeneous(const TypedValue &matrix)                 { return get(matrix, likely_type_heterogeneous); }
    void    setHeterogeneous(const TypedValue &matrix, bool heterogeneous) { setBit(matrix, likely_type_heterogeneous, heterogeneous); }
    TypedValue multiChannel(const TypedValue &matrix)                { return get(matrix, likely_type_multi_channel); }
    void    setMultiChannel(const TypedValue &matrix, bool multiChannel) { setBit(matrix, likely_type_multi_channel, multiChannel); }
    TypedValue multiColumn(const TypedValue &matrix)               { return get(matrix, likely_type_multi_column); }
    void    setMultiColumn(const TypedValue &matrix, bool multiColumn) { setBit(matrix, likely_type_multi_column, multiColumn); }
    TypedValue multiRow(const TypedValue &matrix)            { return get(matrix, likely_type_multi_row); }
    void    setMultiRow(const TypedValue &matrix, bool multiRow) { setBit(matrix, likely_type_multi_row, multiRow); }
    TypedValue multiFrame(const TypedValue &matrix)              { return get(matrix, likely_type_multi_frame); }
    void    setMultiFrame(const TypedValue &matrix, bool multiFrame) { setBit(matrix, likely_type_multi_frame, multiFrame); }
    TypedValue saturation(const TypedValue &matrix)              { return get(matrix, likely_type_saturation); }
    void    setSaturation(const TypedValue &matrix, bool saturation) { setBit(matrix, likely_type_saturation, saturation); }
    TypedValue reserved(const TypedValue &matrix)        { return get(matrix, likely_type_reserved); }
    void    setReserved(const TypedValue &matrix, int reserved) { set(matrix, likely_type_reserved, reserved); }

    TypedValue elements(const TypedValue &matrix)
    {
        return TypedValue(CreateMul(CreateMul(CreateMul(channels(matrix),
                                                        columns(matrix)),
                                              rows(matrix)),
                                    frames(matrix)), likely_type_native);
    }

    TypedValue bytes(const TypedValue &matrix)
    {
        return TypedValue(CreateMul(CreateUDiv(CreateCast(Instruction::ZExt, depth(matrix),
                                                          NativeIntegerType),
                                               constant(8)),
                                    elements(matrix)), likely_type_native);
    }

    TypedValue columnStep(const TypedValue &matrix)
    {
        Value *columnStep = channels(matrix);
        columnStep->setName("cStep");
        return TypedValue(columnStep, likely_type_native);
    }

    TypedValue rowStep(const TypedValue &matrix)
    {
        return TypedValue(CreateMul(columns(matrix),
                                    columnStep(matrix), "rStep"), likely_type_native);
    }

    TypedValue frameStep(const TypedValue &matrix)
    {
        return TypedValue(CreateMul(rows(matrix),
                                    rowStep(matrix), "tStep"), likely_type_native);
    }

    TypedValue index(const TypedValue &matrix, const TypedValue &c)
    {
        if (likely_multi_channel(matrix)) return c;
        else                              return zero();
    }

    TypedValue index(const TypedValue &matrix, const TypedValue &c, const TypedValue &x)
    {
        TypedValue remainder = index(matrix, c);
        if (likely_multi_column(matrix)) return TypedValue(CreateAdd(CreateMul(x, columnStep(matrix)), remainder), likely_type_native);
        else                             return remainder;
    }

    TypedValue index(const TypedValue &matrix, const TypedValue &c, const TypedValue &x, const TypedValue &y)
    {
        TypedValue remainder = index(matrix, c, x);
        if (likely_multi_row(matrix)) return TypedValue(CreateAdd(CreateMul(y, rowStep(matrix)), remainder), likely_type_native);
        else                          return remainder;
    }

    TypedValue index(const TypedValue &matrix, const TypedValue &c, const TypedValue &x, const TypedValue &y, const TypedValue &t)
    {
        TypedValue remainder = index(matrix, c, x, y);
        if (likely_multi_frame(matrix)) return TypedValue(CreateAdd(CreateMul(t, frameStep(matrix)), remainder), likely_type_native);
        else                            return remainder;
    }

    void deindex(const TypedValue &matrix, const TypedValue &i, TypedValue *c)
    {
        (void) matrix;
        *c = i;
    }

    void deindex(const TypedValue &matrix, const TypedValue &i, TypedValue *c, TypedValue *x)
    {
        TypedValue step = columnStep(matrix);
        TypedValue remainder(CreateURem(i, step, "xRem"), likely_type_native);
        *x = TypedValue(CreateExactUDiv(CreateSub(i, remainder), step, "x"), likely_type_native);
        deindex(matrix, remainder, c);
    }

    void deindex(const TypedValue &matrix, const TypedValue &i, TypedValue *c, TypedValue *x, TypedValue *y)
    {
        TypedValue step = rowStep(matrix);
        TypedValue remainder(CreateURem(i, step, "yRem"), likely_type_native);
        *y = TypedValue(CreateExactUDiv(CreateSub(i, remainder), step, "y"), likely_type_native);
        deindex(matrix, remainder, c, x);
    }

    void deindex(const TypedValue &matrix, const TypedValue &i, TypedValue *c, TypedValue *x, TypedValue *y, TypedValue *t)
    {
        TypedValue step = frameStep(matrix);
        TypedValue remainder(CreateURem(i, step, "tRem"), likely_type_native);
        *t = TypedValue(CreateExactUDiv(CreateSub(i, remainder), step, "t"), likely_type_native);
        deindex(matrix, remainder, c, x, y);
    }

    TypedValue GEP(const TypedValue &matrix, Value *i)
    {
        return TypedValue(CreateGEP(data(matrix), i), matrix.type);
    }

    TypedValue load(const TypedValue &matrix, Value *i)
    {
        LoadInst *load = CreateLoad(GEP(matrix, i));
        load->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return TypedValue(load, matrix.type);
    }

    TypedValue store(const TypedValue &matrix, Value *i, const TypedValue &value)
    {
        StoreInst *store = CreateStore(value, GEP(matrix, i));
        store->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return TypedValue(store, matrix.type);
    }

    TypedValue cast(const TypedValue &x, likely_type type)
    {
        if ((x.type & likely_type_mask) == (type & likely_type_mask))
            return x;
        Type *dstType = ty(type);
        return TypedValue(CreateCast(CastInst::getCastOpcode(x, likely_signed(x.type), dstType, likely_signed(type)), x, dstType), type);
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
        loop.body = BasicBlock::Create(getGlobalContext(), "loop_body", function);

        // Create self-referencing loop node
        vector<Value*> metadata;
        MDNode *tmp = MDNode::getTemporary(getGlobalContext(), metadata);
        metadata.push_back(tmp);
        loop.node = MDNode::get(getGlobalContext(), metadata);
        tmp->replaceAllUsesWith(loop.node);
        MDNode::deleteTemporary(tmp);

        // Loops assume there is at least one iteration
        CreateBr(loop.body);
        SetInsertPoint(loop.body);

        loop.i = CreatePHI(NativeIntegerType, 2, "i");
        loop.i->addIncoming(start, entry);

        loops.push(loop);
        return loop;
    }

    void endLoop()
    {
        const Loop &loop = loops.top();
        Value *increment = CreateAdd(loop.i, one(), "loop_increment");
        BasicBlock *loopLatch = BasicBlock::Create(getGlobalContext(), "loop_latch", function);
        CreateBr(loopLatch);
        SetInsertPoint(loopLatch);
        BasicBlock *loopExit = BasicBlock::Create(getGlobalContext(), "loop_exit", function);
        BranchInst *latch = CreateCondBr(CreateICmpEQ(increment, loop.stop, "loop_test"), loopExit, loop.body);
        latch->setMetadata("llvm.loop", loop.node);
        loop.i->addIncoming(increment, loopLatch);
        SetInsertPoint(loopExit);
        loops.pop();
    }

    MDNode *getCurrentNode() { return loops.empty() ? NULL : loops.top().node; }

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

struct KernelInfo
{
    vector<TypedValue> srcs;
    TypedValue i, c, x, y, t;
    likely_type dims;

    KernelInfo(const vector<TypedValue> &srcs) : srcs(srcs) {}

    void init(const vector<TypedValue> &srcs, ExpressionBuilder &builder, const TypedValue &dst, const TypedValue &i)
    {
        this->srcs = srcs;
        this->i = i;
        this->dims = dst.type & likely_type_multi_dimension;
        builder.deindex(dst, i, &c, &x, &y, &t);
    }
};

struct Operation
{
    static map<string, const Operation*> operations;
    static void add(const string &symbol, const Operation *operation)
    {
        operations.insert(pair<string, const Operation*>(symbol, operation));
    }

    virtual ~Operation() {}
    virtual TypedValue call(ExpressionBuilder &builder, const KernelInfo &info, const vector<TypedValue> &args) const = 0;
};
map<string, const Operation*> Operation::operations;

template <class T>
struct RegisterOperation
{
    RegisterOperation(const string &symbol)
    {
        Operation::add(symbol, new T());
    }
};
#define LIKELY_REGISTER_OPERATION(OP, SYM) static struct RegisterOperation<OP##Operation> Register##OP##Operation(SYM);
#define LIKELY_REGISTER(OP) LIKELY_REGISTER_OPERATION(OP, #OP)

class NullaryOperation : public Operation
{
    TypedValue call(ExpressionBuilder &builder, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.empty());
        (void) args;
        return callNullary(builder, info);
    }
    virtual TypedValue callNullary(ExpressionBuilder &builder, const KernelInfo &info) const = 0;
};

#define LIKELY_REGISTER_INDEX(OP)                                                    \
class OP##Operation : public NullaryOperation                                        \
{                                                                                    \
    TypedValue callNullary(ExpressionBuilder &builder, const KernelInfo &info) const \
    {                                                                                \
        (void) builder;                                                              \
        return info.OP;                                                              \
    }                                                                                \
};                                                                                   \
LIKELY_REGISTER(OP)                                                                  \

LIKELY_REGISTER_INDEX(i)
LIKELY_REGISTER_INDEX(c)
LIKELY_REGISTER_INDEX(x)
LIKELY_REGISTER_INDEX(y)
LIKELY_REGISTER_INDEX(t)

class UnaryOperation : public Operation
{
    TypedValue call(ExpressionBuilder &builder, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.size() == 1);
        return callUnary(builder, info, args[0]);
    }
    virtual TypedValue callUnary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue arg) const = 0;
};

class argOperation : public UnaryOperation
{
    TypedValue callUnary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue arg) const
    {
        int index = LLVM_VALUE_TO_INT(arg.value);
        const TypedValue matrix = info.srcs[index];
        TypedValue matrix_i;
        if ((matrix.type & likely_type_multi_dimension) == info.dims) {
            // This matrix has the same dimensionality as the output
            matrix_i = info.i;
        } else {
            matrix_i = builder.index(matrix, info.c, info.x, info.y, info.t);
        }
        return builder.load(matrix, matrix_i);
    }
};
LIKELY_REGISTER(arg)

class typeOperation : public UnaryOperation
{
    TypedValue callUnary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue arg) const
    {
        (void) builder;
        (void) info;
        return ExpressionBuilder::type(arg);
    }
};
LIKELY_REGISTER(type)

class UnaryMathOperation : public UnaryOperation
{
    TypedValue callUnary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue x) const
    {
        (void) info;
        x = builder.cast(x, ExpressionBuilder::validFloatType(x.type));
        vector<Type*> args;
        args.push_back(x.value->getType());
        return TypedValue(builder.CreateCall(Intrinsic::getDeclaration(builder.module, id(), args), x), x.type);
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
    TypedValue call(ExpressionBuilder &builder, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.size() == 2);
        return callBinary(builder, info, args[0], args[1]);
    }
    virtual TypedValue callBinary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue arg1, TypedValue arg2) const = 0;
};

class castOperation : public BinaryOperation
{
    TypedValue callBinary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue x, TypedValue type) const
    {
        (void) info;
        return builder.cast(x, (likely_type)LLVM_VALUE_TO_INT(type.value));
    }
};
LIKELY_REGISTER(cast)

class thresholdOperation : public BinaryOperation
{
    TypedValue callBinary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue x, TypedValue t) const
    {
         (void) info;
         likely_type type = likely_type_from_types(x, t);
         x = builder.cast(x, type);
         t = builder.cast(t, type);
         Value *comp = likely_floating(type) ? builder.CreateFCmpOLE(x, t) : (likely_signed(type) ? builder.CreateICmpSLE(x, t) : builder.CreateICmpULE(x, t));
         TypedValue high = builder.constant(1.0, type);
         TypedValue low = builder.constant(0.0, type);
         return TypedValue(builder.CreateSelect(comp, low, high), type);
    }
};
LIKELY_REGISTER(threshold)

class ArithmeticOperation : public BinaryOperation
{
    TypedValue callBinary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue arg1, TypedValue arg2) const
    {
        (void) info;
        likely_type type = likely_type_from_types(arg1, arg2);
        return callArithmetic(builder, builder.cast(arg1, type), builder.cast(arg2, type), type);
    }
    virtual TypedValue callArithmetic(ExpressionBuilder &builder, TypedValue lhs, TypedValue rhs, likely_type type) const = 0;
};

class addOperation : public ArithmeticOperation
{
    TypedValue callArithmetic(ExpressionBuilder &builder, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(builder.CreateFAdd(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                if (likely_signed(type)) {
                    const int depth = likely_depth(type);
                    Value *result = builder.CreateAdd(lhs, rhs);
                    Value *overflowResult = builder.CreateAdd(builder.CreateLShr(lhs, depth-1), ExpressionBuilder::intMax(depth));
                    Value *overflowCondition = builder.CreateICmpSGE(builder.CreateOr(builder.CreateXor(lhs.value, rhs.value), builder.CreateNot(builder.CreateXor(rhs, result))), ExpressionBuilder::zero(depth));
                    return TypedValue(builder.CreateSelect(overflowCondition, overflowResult, result), type);
                } else {
                    Value *result = builder.CreateAdd(lhs, rhs);
                    Value *overflow = builder.CreateNeg(builder.CreateZExt(builder.CreateICmpULT(result, lhs),
                                                                           Type::getIntNTy(getGlobalContext(), likely_depth(type))));
                    return TypedValue(builder.CreateOr(result, overflow), type);
                }
            } else {
                return TypedValue(builder.CreateAdd(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_OPERATION(add, "+")

class subtractOperation : public ArithmeticOperation
{
    TypedValue callArithmetic(ExpressionBuilder &builder, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(builder.CreateFSub(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                if (likely_signed(type)) {
                    const int depth = likely_depth(type);
                    Value *result = builder.CreateSub(lhs, rhs);
                    Value *overflowResult = builder.CreateAdd(builder.CreateLShr(lhs, depth-1), ExpressionBuilder::intMax(depth));
                    Value *overflowCondition = builder.CreateICmpSLT(builder.CreateAnd(builder.CreateXor(lhs.value, rhs.value), builder.CreateXor(lhs, result)), ExpressionBuilder::zero(depth));
                    return TypedValue(builder.CreateSelect(overflowCondition, overflowResult, result), type);
                } else {
                    Value *result = builder.CreateSub(lhs, rhs);
                    Value *overflow = builder.CreateNeg(builder.CreateZExt(builder.CreateICmpULE(result, lhs),
                                                                           Type::getIntNTy(getGlobalContext(), likely_depth(type))));
                    return TypedValue(builder.CreateAnd(result, overflow), type);
                }
            } else {
                return TypedValue(builder.CreateSub(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_OPERATION(subtract, "-")

class multiplyOperation : public ArithmeticOperation
{
    TypedValue callArithmetic(ExpressionBuilder &builder, TypedValue lhs, TypedValue rhs, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(builder.CreateFMul(lhs, rhs), type);
        } else {
            if (likely_saturation(type)) {
                const int depth = likely_depth(type);
                Type *originalType = Type::getIntNTy(getGlobalContext(), depth);
                Type *extendedType = Type::getIntNTy(getGlobalContext(), 2*depth);
                Value *result = builder.CreateMul(builder.CreateZExt(lhs, extendedType),
                                                  builder.CreateZExt(rhs, extendedType));
                Value *lo = builder.CreateTrunc(result, originalType);

                if (likely_signed(type)) {
                    Value *hi = builder.CreateTrunc(builder.CreateAShr(result, depth), originalType);
                    Value *overflowResult = builder.CreateAdd(builder.CreateLShr(builder.CreateXor(lhs.value, rhs.value), depth-1), ExpressionBuilder::intMax(depth));
                    Value *overflowCondition = builder.CreateICmpNE(hi, builder.CreateAShr(lo, depth-1));
                    return TypedValue(builder.CreateSelect(overflowCondition, overflowResult, builder.CreateTrunc(result, lhs.value->getType())), type);
                } else {
                    Value *hi = builder.CreateTrunc(builder.CreateLShr(result, depth), originalType);
                    Value *overflow = builder.CreateNeg(builder.CreateZExt(builder.CreateICmpNE(hi, ExpressionBuilder::zero(depth)), originalType));
                    return TypedValue(builder.CreateOr(lo, overflow), type);
                }
            } else {
                return TypedValue(builder.CreateMul(lhs, rhs), type);
            }
        }
    }
};
LIKELY_REGISTER_OPERATION(multiply, "*")

class divideOperation : public ArithmeticOperation
{
    TypedValue callArithmetic(ExpressionBuilder &builder, TypedValue n, TypedValue d, likely_type type) const
    {
        if (likely_floating(type)) {
            return TypedValue(builder.CreateFDiv(n, d), type);
        } else {
            if (likely_signed(type)) {
                if (likely_saturation(type)) {
                    const int depth = likely_depth(type);
                    Value *safe_i = builder.CreateAdd(n, builder.CreateZExt(builder.CreateICmpNE(builder.CreateOr(builder.CreateAdd(d, ExpressionBuilder::constant(1, depth)), builder.CreateAdd(n, ExpressionBuilder::intMin(depth))), ExpressionBuilder::zero(depth)), n.value->getType()));
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

#define LIKELY_REGISTER_COMPARISON(OP, SYM)                                                                         \
class OP##Operation : public ArithmeticOperation                                                                    \
{                                                                                                                   \
    TypedValue callArithmetic(ExpressionBuilder &builder, TypedValue lhs, TypedValue rhs, likely_type type) const   \
    {                                                                                                               \
        return TypedValue(likely_floating(type) ? builder.CreateFCmpO##OP(lhs, rhs)                                 \
                                                : (likely_signed(type) ? builder.CreateICmpS##OP(lhs, rhs)          \
                                                                       : builder.CreateICmpU##OP(lhs, rhs)), type); \
    }                                                                                                               \
};                                                                                                                  \
LIKELY_REGISTER_OPERATION(OP, SYM)                                                                                  \

LIKELY_REGISTER_COMPARISON(LT, "<")
LIKELY_REGISTER_COMPARISON(LE, "<=")
LIKELY_REGISTER_COMPARISON(GT, ">")
LIKELY_REGISTER_COMPARISON(GE, ">=")

#define LIKELY_REGISTER_EQUALITY(OP, SYM)                                                                         \
class OP##Operation : public ArithmeticOperation                                                                  \
{                                                                                                                 \
    TypedValue callArithmetic(ExpressionBuilder &builder, TypedValue lhs, TypedValue rhs, likely_type type) const \
    {                                                                                                             \
        return TypedValue(likely_floating(type) ? builder.CreateFCmpO##OP(lhs, rhs)                               \
                                                : builder.CreateICmp##OP(lhs, rhs), type);                        \
    }                                                                                                             \
};                                                                                                                \
LIKELY_REGISTER_OPERATION(OP, SYM)                                                                                \

LIKELY_REGISTER_EQUALITY(EQ, "==")
LIKELY_REGISTER_EQUALITY(NE, "!=")

class BinaryMathOperation : public BinaryOperation
{
    TypedValue callBinary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue x, TypedValue n) const
    {
        (void) info;
        const likely_type type = nIsInteger() ? x.type : likely_type_from_types(x, n);
        x = builder.cast(x, ExpressionBuilder::validFloatType(type));
        n = builder.cast(n, nIsInteger() ? likely_type_i32 : x.type);
        vector<Type*> args;
        args.push_back(x.value->getType());
        return TypedValue(builder.CreateCall2(Intrinsic::getDeclaration(builder.module, id(), args), x, n), x.type);
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
    TypedValue call(ExpressionBuilder &builder, const KernelInfo &info, const vector<TypedValue> &args) const
    {
        assert(args.size() == 3);
        return callTernary(builder, info, args[0], args[1], args[2]);
    }
    virtual TypedValue callTernary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue arg1, TypedValue arg2, TypedValue arg3) const = 0;
};

class fmaOperation : public TernaryOperation
{
    TypedValue callTernary(ExpressionBuilder &builder, const KernelInfo &info, TypedValue a, TypedValue x, TypedValue c) const
    {
        (void) info;
        const likely_type type = likely_type_from_types(likely_type_from_types(a, x), c);
        x = builder.cast(x, ExpressionBuilder::validFloatType(type));
        a = builder.cast(a, x.type);
        c = builder.cast(c, x.type);
        vector<Type*> args;
        args.push_back(x.value->getType());
        return TypedValue(builder.CreateCall3(Intrinsic::getDeclaration(builder.module, Intrinsic::fma, args), x, a, c), x.type);
    }
};
LIKELY_REGISTER(fma)

struct LikelyKernelOptimizationPass : public FunctionPass
{
    static char ID;
    LikelyKernelOptimizationPass() : FunctionPass(ID) {}

    struct MatrixInfo
    {
        Value *channels, *columns, *rows, *frames, *columnStep, *rowStep, *frameStep;
		MatrixInfo()
		  : channels(NULL), columns(NULL), rows(NULL), frames(NULL), columnStep(NULL), rowStep(NULL), frameStep(NULL)
		{}
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

// Parallel synchronization
static condition_variable worker;
static mutex work;
static atomic<bool> *workers = NULL;
static size_t numWorkers = 0;

// Parallel data
static void *currentThunk = NULL;
static likely_arity thunkArity = 0;
static likely_size thunkSize = 0;
static likely_const_mat thunkMatricies[LIKELY_NUM_ARITIES+1];

static void executeWorker(size_t id)
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

static void workerThread(size_t id)
{
    while (true) {
        {
            unique_lock<mutex> lock(work);
            workers[id] = false;
            while (!workers[id])
                worker.wait(lock);
        }

        executeWorker(id);
    }
}

struct JITResources
{
    string name;
    Module *module;
    ExecutionEngine *executionEngine;
    TargetMachine *targetMachine;

    JITResources()
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

        static int index = 0;
        stringstream stream; stream << "likely_" << index++;
        name = stream.str();

        module = new Module(name, getGlobalContext());
        likely_assert(module != NULL, "failed to create LLVM Module");
        string targetTriple = sys::getProcessTriple();
#ifdef _WIN32
        targetTriple = targetTriple + "-elf";
#endif
        module->setTargetTriple(targetTriple);

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

    ~JITResources()
    {
        delete targetMachine;
        delete executionEngine;
        delete module;
    }
};

struct FunctionBuilder : private JITResources
{
    likely_type *type;
    void *f;

    FunctionBuilder(likely_ir ir, const vector<likely_type> &types)
    {
        type = new likely_type[types.size()];
        memcpy(type, types.data(), sizeof(likely_type) * types.size());

        Function *function = getFunction(name, module, (likely_arity)types.size(), PointerType::getUnqual(TheMatrixStruct));
        vector<TypedValue> srcs = getArgs(function, types);
        ExpressionBuilder builder(module, function);
        KernelInfo info(srcs);

        TypedValue dstChannels = getDimensions(builder, info, ir, "channels", srcs.size() > 0 ? srcs[0] : TypedValue());
        TypedValue dstColumns  = getDimensions(builder, info, ir, "columns" , srcs.size() > 0 ? srcs[0] : TypedValue());
        TypedValue dstRows     = getDimensions(builder, info, ir, "rows"    , srcs.size() > 0 ? srcs[0] : TypedValue());
        TypedValue dstFrames   = getDimensions(builder, info, ir, "frames"  , srcs.size() > 0 ? srcs[0] : TypedValue());

        Function *thunk;
        likely_type dstType;
        {
            thunk = getFunction(name+"_thunk", module, (likely_arity)types.size(), Type::getVoidTy(getGlobalContext()), PointerType::getUnqual(TheMatrixStruct), NativeIntegerType, NativeIntegerType);
            vector<TypedValue> srcs = getArgs(thunk, types);
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

            ExpressionBuilder builder(module, thunk);
            KernelInfo info(srcs);
            TypedValue i = TypedValue(builder.beginLoop(builder.entry, start, stop).i, likely_type_native);
            info.init(srcs, builder, dst, i);

            TypedValue result = getExpression(builder, info, ir);

            dstType = result.type;
            builder.store(TypedValue(dst.value, dstType), i, result);
            builder.endLoop();
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

        std::vector<Value*> likelyNewArgs;
        likelyNewArgs.push_back(builder.type(dstType));
        likelyNewArgs.push_back(dstChannels);
        likelyNewArgs.push_back(dstColumns);
        likelyNewArgs.push_back(dstRows);
        likelyNewArgs.push_back(dstFrames);
        likelyNewArgs.push_back(ConstantPointerNull::get(Type::getInt8PtrTy(getGlobalContext())));
        likelyNewArgs.push_back(builder.constant(0, 8));
        Value *dst = builder.CreateCall(likelyNew, likelyNewArgs);

        // An impossible case used to ensure that `likely_new` isn't stripped when optimizing executable size
        if (likelyNew == NULL)
            likely_new(likely_type_null, 0, 0, 0, 0, NULL, 0);

        Value *kernelSize = builder.CreateMul(builder.CreateMul(builder.CreateMul(dstChannels, dstColumns), dstRows), dstFrames);

        if (!types.empty() && likely_parallel(types[0])) {
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
            likelyForkArgs.push_back(ExpressionBuilder::constant((double)types.size(), 8));
            likelyForkArgs.push_back(kernelSize);
            likelyForkArgs.insert(likelyForkArgs.end(), srcs.begin(), srcs.end());
            likelyForkArgs.push_back(dst);
            builder.CreateCall(likelyFork, likelyForkArgs);

            // An impossible case used to ensure that `likely_fork` isn't stripped when optimizing executable size
            if (likelyFork == NULL)
                likely_fork(NULL, 0, 0, NULL);
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

        FunctionPassManager functionPassManager(module);
        functionPassManager.add(createVerifierPass(PrintMessageAction));
        functionPassManager.add(createInstructionCombiningPass());
        functionPassManager.run(*function);

//        module->dump();
        executionEngine->finalizeObject();
        f = executionEngine->getPointerToFunction(function);
    }

    ~FunctionBuilder()
    {
        delete[] type;
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
            function->setDoesNotAlias((unsigned int)i+1);
            function->setDoesNotCapture((unsigned int)i+1);
        }
        return function;
    }

    static vector<TypedValue> getArgs(Function *function, const vector<likely_type> &types)
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

    static TypedValue getDimensions(ExpressionBuilder &builder, const KernelInfo &info, likely_ir ir, const char *axis, const TypedValue &arg0)
    {
        lua_getfield(ir, -1, axis);
        Value *result;
        if (lua_isnil(ir, -1)) {
            if (arg0.isNull()) {
                result = ExpressionBuilder::constant(1);
            } else {
                if      (!strcmp(axis, "channels")) result = builder.channels(arg0);
                else if (!strcmp(axis, "columns"))  result = builder.columns (arg0);
                else if (!strcmp(axis, "rows"))     result = builder.rows    (arg0);
                else                                result = builder.frames  (arg0);
            }
        } else {
            result = builder.cast(getExpression(builder, info, ir), likely_type_native);
        }

        likely_type type = likely_type_native;
        const bool isMulti = (!LLVM_VALUE_IS_INT(result)) || (LLVM_VALUE_TO_INT(result) > 1);
        if      (!strcmp(axis, "channels")) likely_set_multi_channel(&type, isMulti);
        else if (!strcmp(axis, "columns"))  likely_set_multi_column (&type, isMulti);
        else if (!strcmp(axis, "rows"))     likely_set_multi_row    (&type, isMulti);
        else                                likely_set_multi_frame  (&type, isMulti);

        lua_pop(ir, 1);
        return TypedValue(result, type);
    }

    static TypedValue getConstant(const string &str, bool *ok)
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
        return ExpressionBuilder::constant(value, type);
    }

    static TypedValue getExpression(ExpressionBuilder &builder, const KernelInfo &info, likely_ir ir)
    {
        string operator_;
        vector<TypedValue> operands;
        if (lua_istable(ir, -1)) {
            lua_rawgeti(ir, -1, 1);
            operator_ = lua_tostring(ir, -1);
            lua_pop(ir, 1);
            int index = 2;
            bool done = false;
            while (!done) {
                lua_rawgeti(ir, -1, index++);
                if (!lua_isnil(ir, -1)) operands.push_back(getExpression(builder, info, ir));
                else                    done = true;
                lua_pop(ir, 1);
            }
        } else {
            operator_ = lua_tostring(ir, -1);
        }

        map<string,const Operation*>::iterator it = Operation::operations.find(operator_);
        if (it != Operation::operations.end())
            return it->second->call(builder, info, operands);

        bool ok;
        TypedValue c = getConstant(operator_, &ok);
        if (!ok)
            c = ExpressionBuilder::constant(likely_type_from_string(operator_.c_str()), likely_type_u32);
        likely_assert(c.type != likely_type_null, "unrecognized literal: %s", operator_.c_str());
        return c;

        return TypedValue();
    }
};

} // namespace (anonymous)

struct VTable : public JITResources
{
    static PointerType *vtableType;
    likely_ir ir;
    likely_arity n;
    vector<FunctionBuilder*> functions;
    Function *likelyDispatch;

    VTable(likely_ir ir_)
        : ir(ir_)
    {
        n = computeArityRecursive(ir);

        if (vtableType == NULL)
            vtableType = PointerType::getUnqual(StructType::create(getGlobalContext(), "VTable"));

        static FunctionType *LikelyDispatchSignature = NULL;
        if (LikelyDispatchSignature == NULL) {
            Type *dispatchReturn = PointerType::getUnqual(TheMatrixStruct);
            vector<Type*> dispatchParameters;
            dispatchParameters.push_back(vtableType);
            dispatchParameters.push_back(PointerType::getUnqual(PointerType::getUnqual(TheMatrixStruct)));
            LikelyDispatchSignature = FunctionType::get(dispatchReturn, dispatchParameters, false);
        }
        likelyDispatch = Function::Create(LikelyDispatchSignature, GlobalValue::ExternalLinkage, "likely_dispatch", module);
        likelyDispatch->setCallingConv(CallingConv::C);
        likelyDispatch->setDoesNotAlias(0);
        likelyDispatch->setDoesNotAlias(1);
        likelyDispatch->setDoesNotAlias(2);
        likelyDispatch->setDoesNotCapture(1);
        likelyDispatch->setDoesNotCapture(2);

        // An impossible case used to ensure that `likely_dispatch` isn't stripped when optimizing executable size
        if (likelyDispatch == NULL)
            likely_dispatch(NULL, NULL);
    }

    ~VTable()
    {
        lua_close(ir);
        for (FunctionBuilder *function : functions)
            delete function;
    }

    likely_function compile() const
    {
        static FunctionType* functionType = NULL;
        if (functionType == NULL) {
            Type *functionReturn = PointerType::getUnqual(TheMatrixStruct);
            vector<Type*> functionParameters;
            functionParameters.push_back(PointerType::getUnqual(TheMatrixStruct));
            functionType = FunctionType::get(functionReturn, functionParameters, true);
        }
        Function *function = getFunction(functionType);
        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);

        Value *array;
        if (n > 0) {
            array = builder.CreateAlloca(PointerType::getUnqual(TheMatrixStruct), Constant::getIntegerValue(Type::getInt32Ty(getGlobalContext()), APInt(32, (uint64_t)n)));
            builder.CreateStore(function->arg_begin(), builder.CreateGEP(array, Constant::getIntegerValue(NativeIntegerType, APInt(8*sizeof(void*), 0))));
            if (n > 1) {
                Value *vaList = builder.CreateAlloca(IntegerType::getInt8PtrTy(getGlobalContext()));
                Value *vaListRef = builder.CreateBitCast(vaList, Type::getInt8PtrTy(getGlobalContext()));
                builder.CreateCall(Intrinsic::getDeclaration(module, Intrinsic::vastart), vaListRef);
                for (likely_arity i=1; i<n; i++)
                    builder.CreateStore(builder.CreateVAArg(vaList, PointerType::getUnqual(TheMatrixStruct)), builder.CreateGEP(array, Constant::getIntegerValue(NativeIntegerType, APInt(8*sizeof(void*), i))));
                builder.CreateCall(Intrinsic::getDeclaration(module, Intrinsic::vaend), vaListRef);
            }
        } else {
            array = ConstantPointerNull::get(PointerType::getUnqual(PointerType::getUnqual(TheMatrixStruct)));
        }
        builder.CreateRet(builder.CreateCall2(likelyDispatch, thisVTable(), array));

        return reinterpret_cast<likely_function>(finalize(function));
    }

    likely_function_n compileN() const
    {
        static FunctionType* functionType = NULL;
        if (functionType == NULL) {
            Type *functionReturn = PointerType::getUnqual(TheMatrixStruct);
            vector<Type*> functionParameters;
            functionParameters.push_back(PointerType::getUnqual(PointerType::getUnqual(TheMatrixStruct)));
            functionType = FunctionType::get(functionReturn, functionParameters, true);
        }
        Function *function = getFunction(functionType);
        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);
        builder.CreateRet(builder.CreateCall2(likelyDispatch, thisVTable(), function->arg_begin()));
        return reinterpret_cast<likely_function_n>(finalize(function));
    }

private:
    static likely_arity computeArityRecursive(lua_State *L)
    {
        if (!lua_istable(L, -1))
            return 0;

        lua_pushinteger(L, 1);
        lua_gettable(L, -2);
        if (!strcmp(lua_tostring(L, -1), "arg")) {
            lua_pushinteger(L, 2);
            lua_gettable(L, -3);
            likely_arity n = (likely_arity) lua_tointeger(L, -1) + 1;
            lua_pop(L, 2);
            return n;
        } else {
            lua_pop(L, 1);
        }

        likely_arity n = 0;
        const int len = luaL_len(L, -1);
        for (int i=1; i<=len; i++) {
            lua_pushinteger(L, i);
            lua_gettable(L, -2);
            n = max(n, computeArityRecursive(L));
            lua_pop(L, 1);
        }
        return n;
    }

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
        return ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(getGlobalContext(), 8*sizeof(this)), uintptr_t(this)), vtableType);
    }

    void *finalize(Function *function) const
    {
        FunctionPassManager functionPassManager(module);
        functionPassManager.add(createVerifierPass(PrintMessageAction));
        functionPassManager.run(*function);
        executionEngine->finalizeObject();
        return executionEngine->getPointerToFunction(function);
    }
};
PointerType *VTable::vtableType = NULL;

likely_mat likely_dispatch(struct VTable *vtable, likely_mat *m)
{
    void *function = NULL;
    for (size_t i=0; i<vtable->functions.size(); i++) {
        const FunctionBuilder *functionBuilder = vtable->functions[i];
        for (likely_arity j=0; j<vtable->n; j++)
            if (m[j]->type != functionBuilder->type[j])
                goto Next;
        function = functionBuilder->f;
        break;
    Next:
        continue;
    }

    if (function == NULL) {
        vector<likely_type> types;
        for (int i=0; i<vtable->n; i++)
            types.push_back(m[i]->type);
        FunctionBuilder *functionBuilder = new FunctionBuilder(vtable->ir, types);
        vtable->functions.push_back(functionBuilder);
        function = vtable->functions.back()->f;
    }

    typedef likely_mat (*f0)(void);
    typedef likely_mat (*f1)(likely_const_mat);
    typedef likely_mat (*f2)(likely_const_mat, likely_const_mat);
    typedef likely_mat (*f3)(likely_const_mat, likely_const_mat, likely_const_mat);

    likely_mat dst;
    switch (vtable->n) {
      case 0: dst = reinterpret_cast<f0>(function)(); break;
      case 1: dst = reinterpret_cast<f1>(function)(m[0]); break;
      case 2: dst = reinterpret_cast<f2>(function)(m[0], m[1]); break;
      case 3: dst = reinterpret_cast<f3>(function)(m[0], m[1], m[2]); break;
      default: dst = NULL; likely_assert(false, "likely_dispatch invalid arity: %d", vtable->n);
    }

    return dst;
}

void likely_fork(void *thunk, likely_arity arity, likely_size size, likely_const_mat src, ...)
{
    static mutex forkLock;
    lock_guard<mutex> lockFork(forkLock);

    // Spin up the worker threads
    if (workers == NULL) {
        numWorkers = std::max((int)thread::hardware_concurrency(), 1);
        workers = new atomic<bool>[numWorkers];
        for (size_t i = 1; i < numWorkers; i++) {
            workers[i] = true;
            thread(workerThread, i).detach();
            while (workers[i]) {} // Wait for the worker to initialize
        }
    }

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

    {
        unique_lock<mutex> lock(work);
        for (size_t i = 1; i < numWorkers; i++) {
            assert(!workers[i]);
            workers[i] = true;
        }
    }

    worker.notify_all();
    executeWorker(0);

    for (size_t i = 1; i < numWorkers; i++)
        while (workers[i]) {} // Wait for the worker to finish
}

likely_ir likely_ir_from_string(const char *str)
{
    likely_ir L = luaL_newstate();
    luaL_dostring(L, (string("return ") + str).c_str());
    const int args = lua_gettop(L);
    likely_assert(args == 1, "'likely_ir_from_string' expected one result, got: %d", args);
    likely_assert(lua_istable(L, 1), "'likely_ir_from_string' expected a table result");
    return L;
}

static void toStream(lua_State *L, int index, stringstream &stream, int levels = 1)
{
    lua_pushvalue(L, index);
    const int type = lua_type(L, -1);
    if (type == LUA_TBOOLEAN) {
        stream << (lua_toboolean(L, -1) ? "true" : "false");
    } else if (type == LUA_TNUMBER) {
        stream << lua_tonumber(L, -1);
    } else if (type == LUA_TSTRING) {
        stream << "\"" << lua_tostring(L, -1) << "\"";
    } else if (type == LUA_TTABLE) {
        if (levels == 0) {
            stream << "table";
        } else {
            map<int,string> integers;
            map<string,string> strings;
            lua_pushnil(L);
            while (lua_next(L, -2)) {
                lua_pushvalue(L, -2);
                int isnum;
                int key = lua_tointegerx(L, -1, &isnum);
                lua_pop(L, 1);
                stringstream value;
                toStream(L, -1, value, levels - 1);
                if (isnum) {
                    integers.insert(pair<int,string>(key, value.str()));
                } else {
                    lua_pushvalue(L, -2);
                    strings.insert(pair<string,string>(lua_tostring(L, -1), value.str()));
                    lua_pop(L, 1);
                }
                lua_pop(L, 1);
            }

            stream << "{";
            int expectedIndex = 1;
            for (map<int,string>::iterator iter = integers.begin(); iter != integers.end(); iter++) {
                if (iter != integers.begin())
                    stream << ", ";
                if (iter->first == expectedIndex) {
                    stream << iter->second;
                    expectedIndex++;
                } else {
                    stream << iter->first << "=" << iter->second;
                }
            }

            for (map<string,string>::iterator iter = strings.begin(); iter != strings.end(); iter++) {
                if (iter != strings.begin() || !integers.empty())
                    stream << ", ";
                stream << iter->first << "=" << iter->second;
            }
            stream << "}";
        }
    } else {
        stream << lua_typename(L, type);
    }
    lua_pop(L, 1);
}

const char *likely_ir_to_string(likely_ir ir)
{
    static string result;
    stringstream stream;
    toStream(ir, -1, stream, std::numeric_limits<int>::max());
    result = stream.str();
    return result.c_str();
}

likely_function likely_compile(likely_ir ir)
{
    return (new VTable(ir))->compile();
}

likely_function_n likely_compile_n(likely_ir ir)
{
    return (new VTable(ir))->compileN();
}

void likely_stack_dump(lua_State *L, int levels)
{
    if (levels == 0)
        return;

    stringstream stream;
    const int top = lua_gettop(L);
    for (int i=1; i<=top; i++) {
        stream << i << ": ";
        toStream(L, i, stream, levels);
        stream << "\n";
    }
    fprintf(stderr, "Lua stack dump:\n%s", stream.str().c_str());
    abort();
}
