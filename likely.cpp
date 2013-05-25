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
#include <llvm/ExecutionEngine/JIT.h>
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
#include <iomanip>
#include <iostream>
#include <mutex>
#include <regex>
#include <stack>
#include <sstream>
#include <thread>

#include "likely.h"

using namespace llvm;
using namespace std;

#define LIKELY_NUM_ARITIES 4

static Module *TheModule = NULL;
static StructType *TheMatrixStruct = NULL;

void likely_allocate(likely_matrix *m)
{
    m->data = (uint8_t*)malloc(likely_bytes(m));
}

void likely_free(likely_matrix *m)
{
    free(m->data);
}

double likely_element(const likely_matrix *m, uint32_t c, uint32_t x, uint32_t y, uint32_t t)
{
    likely_assert(m != NULL, "likely_element received a null matrix");
    const int columnStep = m->channels;
    const int rowStep = m->columns * columnStep;
    const int frameStep = m->rows * rowStep;
    const int index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (likely_type(m->hash)) {
      case likely_hash_u8:  return  ((uint8_t*)m->data)[index];
      case likely_hash_u16: return ((uint16_t*)m->data)[index];
      case likely_hash_u32: return ((uint32_t*)m->data)[index];
      case likely_hash_u64: return ((uint64_t*)m->data)[index];
      case likely_hash_i8:  return   ((int8_t*)m->data)[index];
      case likely_hash_i16: return  ((int16_t*)m->data)[index];
      case likely_hash_i32: return  ((int32_t*)m->data)[index];
      case likely_hash_i64: return  ((int64_t*)m->data)[index];
      case likely_hash_f32: return    ((float*)m->data)[index];
      case likely_hash_f64: return   ((double*)m->data)[index];
      default: likely_assert(false, "likely_element unsupported type");
    }
    return numeric_limits<double>::quiet_NaN();
}

void likely_set_element(likely_matrix *m, double value, uint32_t c, uint32_t x, uint32_t y, uint32_t t)
{
    likely_assert(m != NULL, "likely_set_element received a null matrix");
    const int columnStep = m->channels;
    const int rowStep = m->channels * columnStep;
    const int frameStep = m->rows * rowStep;
    const int index = t*frameStep + y*rowStep + x*columnStep + c;

    switch (likely_type(m->hash)) {
      case likely_hash_u8:   ((uint8_t*)m->data)[index] = value; break;
      case likely_hash_u16: ((uint16_t*)m->data)[index] = value; break;
      case likely_hash_u32: ((uint32_t*)m->data)[index] = value; break;
      case likely_hash_u64: ((uint64_t*)m->data)[index] = value; break;
      case likely_hash_i8:    ((int8_t*)m->data)[index] = value; break;
      case likely_hash_i16:  ((int16_t*)m->data)[index] = value; break;
      case likely_hash_i32:  ((int32_t*)m->data)[index] = value; break;
      case likely_hash_i64:  ((int64_t*)m->data)[index] = value; break;
      case likely_hash_f32:    ((float*)m->data)[index] = value; break;
      case likely_hash_f64:   ((double*)m->data)[index] = value; break;
      default: likely_assert(false, "likely_set_element unsupported type");
    }
}

void likely_print_matrix(const likely_matrix *m)
{
    if ((m == NULL) || (m->data == NULL)) return;
    const int type = likely_type(m->hash);
    for (uint t=0; t<m->frames; t++) {
        for (uint y=0; y<m->rows; y++) {
            cout << (m->rows > 1 ? (y == 0 ? "[" : " ") : "");
            for (uint x=0; x<m->columns; x++) {
                for (uint c=0; c<m->channels; c++) {
                    const double value = likely_element(m, c, x, y, t);
                    switch (type) {
                      case likely_hash_u8:  cout <<  (uint8_t)value; break;
                      case likely_hash_u16: cout << (uint16_t)value; break;
                      case likely_hash_u32: cout << (uint32_t)value; break;
                      case likely_hash_u64: cout << (uint64_t)value; break;
                      case likely_hash_i8:  cout <<   (int8_t)value; break;
                      case likely_hash_i16: cout <<  (int16_t)value; break;
                      case likely_hash_i32: cout <<  (int32_t)value; break;
                      case likely_hash_i64: cout <<  (int64_t)value; break;
                      case likely_hash_f32: cout <<    (float)value; break;
                      case likely_hash_f64: cout <<   (double)value; break;
                      default: likely_assert(false, "likely_print_matrix unsupported type.");
                    }
                }
                cout << (m->channels > 1 ? ";" : (x < m->columns-1 ? " " : ""));
            }
            cout << ((m->columns > 1) && (y < m->rows-1) ? "\n" : "");
        }
        cout << (m->rows > 1 ? "]\n" : "");
        cout << (t < m->frames-1 ? "\n" : "");
    }
}

void likely_assert(bool condition, const char *format, ...)
{
    if (condition) return;
    fprintf(stderr, "LIKELY ERROR - ");
    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    fprintf(stderr, ".\n");
    abort();
}

void likely_dump()
{
    TheModule->dump();
}

namespace likely
{

static vector<string> split(const string &s, char delim)
{
    vector<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim))
        elems.push_back(item);
    return elems;
}

static int indexOf(const vector<string> &v, const string &s)
{
    auto index = find(v.begin(), v.end(), s);
    if (index == v.end()) return -1;
    return distance(v.begin(), index);
}

struct Node
{
    string value;
    vector<Node> children;

    Node() = default;
    Node(const string &value_) : value(value_) {}
    Node(const string &value_, const vector<Node> &children_) : value(value_), children(children_) {}
    Node(const string &value_, const Node &node) : value(value_) { children.push_back(node); }
    Node(const string &value_, const Node &nodeA, const Node &nodeB)
        : value(value_) { children.push_back(nodeA); children.push_back(nodeB); }
};

struct Definition
{
    string name, documentation;
    vector<string> parameters;
    Node equation;

    Definition() = default;
    Definition(const smatch &sm)
    {
        likely_assert(sm.size() == 5, "Definition::Definition expected 5 fields, got: %zu", sm.size());

        name = sm[2];
        parameters = split(string(sm[3]).substr(1, string(sm[3]).size()-2), ',');
        documentation = sm[4];

        // Equation
        static const regex syntax("^\\s*([+\\-*/]|\\w+).*$");
        vector<string> tokens;
        string unparsed = sm[1];
        while (!unparsed.empty()) {
            smatch sm;
            regex_match(unparsed, sm, syntax);
            likely_assert(sm.size() >= 2, "Definition::Definition unable to tokenize: %s", unparsed.c_str());

            const string &token = sm[1];
            tokens.push_back(token);
            unparsed = unparsed.substr(unparsed.find(token.c_str())+token.size());
        }
        likely_assert(getEquation(tokens, equation), "Definition::Definition unable to parse: %s", string(sm[1]).c_str());
    }

    static Definition get(const string &name)
    {
        // Parse likely_index_html for definitions
        if (definitions.size() == 0)
            for (const Definition &definition : Definition::definitionsFromString(indexHTML()))
                definitions[definition.name] = definition;

        Definition definition = definitions[name];
        likely_assert(name == definition.name, "Definition::get missing definition for: %s", name.c_str());

        return definition;
    }

private:
    static map<string,Definition> definitions;

    static vector<Definition> definitionsFromString(const string &str)
    {
        static const string begin = "<div class=\"likely\">";
        static const string end = "</div>";
        static const regex  syntax("\\s*\\$(.*)\\$\\s*<h4>(.*)<small>(.*)</small></h4>\\s*<p>(.*)</p>\\s*");

        vector<Definition> definitions;
        size_t startDefinition = str.find(begin.c_str());
        while (startDefinition != string::npos) {
            size_t endDefinition = str.find(end.c_str(), startDefinition+begin.length());
            likely_assert(endDefinition != string::npos, "Definition::definitionsFromString unclosed definition, missing: %s", end.c_str());

            const string definition = str.substr(startDefinition+begin.length(), endDefinition-startDefinition-begin.length());
            smatch sm;
            likely_assert(regex_match(definition, sm, syntax), "Definition::definitionsFromString invalid definition: %s", definition.c_str());

            definitions.push_back(Definition(sm));
            startDefinition = str.find(begin.c_str(), endDefinition+1, begin.length());
        }

        return definitions;
    }

    static bool getEquation(const vector<string> &tokens, Node &equation)
    {
        for (size_t i=1; i<tokens.size()-1; i++)
            if ((tokens[i] == "+") || (tokens[i] == "-")) {
                Node lhs, rhs;
                if (!getEquation(vector<string>(tokens.begin(), tokens.begin()+i), lhs)) continue;
                if (!getEquation(vector<string>(tokens.begin()+i+1, tokens.end()), rhs)) continue;
                equation = Node(tokens[i], lhs, rhs);
                return true;
            }
        return getTerm(tokens, equation);
    }

    static bool getTerm(const vector<string> &tokens, Node &term)
    {
        for (size_t i=1; i<tokens.size()-1; i++)
            if ((tokens[i] == "*") || (tokens[i] == "/")) {
                Node lhs, rhs;
                if (!getTerm(vector<string>(tokens.begin(), tokens.begin()+i), lhs)) continue;
                if (!getTerm(vector<string>(tokens.begin()+i+1, tokens.end()), rhs)) continue;
                term = Node(tokens[i], lhs, rhs);
                return true;
            }
        return getFactor(tokens, term);
    }

    static bool getFactor(const vector<string> &tokens, Node &factor)
    {
        if (getMatrix(tokens, factor)) return true;
        if (getParameter(tokens, factor)) return true;
        return getNumber(tokens, factor);
    }

    static bool getMatrix(const vector<string> &tokens, Node &matrix)
    {
        static const regex syntax("src[0-9]?");
        return getTerminal(tokens, matrix, syntax);
    }

    static bool getParameter(const vector<string> &tokens, Node &parameter)
    {
        static const regex syntax("[a-z]+");
        return getTerminal(tokens, parameter, syntax);
    }

    static bool getNumber(const vector<string> &tokens, Node &number)
    {
        static const regex syntax("[0-9\\.]+");
        return getTerminal(tokens, number, syntax);
    }

    static bool getTerminal(const vector<string> &tokens, Node &terminal, const regex &syntax)
    {
        if (tokens.size() != 1) return false;
        smatch sm;
        if (!regex_match(tokens[0], sm, syntax)) return false;
        terminal = Node(tokens[0]);
        return true;
    }
};

map<string, Definition> Definition::definitions;

struct MatrixBuilder
{
    IRBuilder<> *b;
    Function *f;
    Twine n;
    likely_hash h;
    Value *v;

    struct Loop {
        BasicBlock *body, *exit;
        PHINode *i;
        Value *stop;
        MDNode *node;
    };
    stack<Loop> loops;

    MatrixBuilder() : b(NULL), f(NULL), h(likely_hash_null), v(NULL) {}
    MatrixBuilder(IRBuilder<> *builder, Function *function, const Twine &name, likely_hash hash = likely_hash_null, Value *value = NULL)
        : b(builder), f(function), n(name), h(hash), v(value) {}
    void reset(IRBuilder<> *builder, Function *function, Value *value) { b = builder; f = function; v = value; }

    static Constant *constant(int value, int bits = 32) { return Constant::getIntegerValue(Type::getInt32Ty(getGlobalContext()), APInt(bits, value)); }
    static Constant *constant(bool value) { return constant(value, 1); }
    static Constant *constant(float value) { return ConstantFP::get(Type::getFloatTy(getGlobalContext()), value == 0 ? -0.0f : value); }
    static Constant *constant(double value) { return ConstantFP::get(Type::getDoubleTy(getGlobalContext()), value == 0 ? -0.0 : value); }
    static Constant *constant(const char *value) { return ConstantExpr::getIntToPtr(ConstantInt::get(IntegerType::get(getGlobalContext(), 8*sizeof(value)), uint64_t(value)), Type::getInt8PtrTy(getGlobalContext())); }
    static Constant *zero() { return constant(0); }
    static Constant *one() { return constant(1); }
    Constant *autoConstant(double value) const { return likely_is_floating(h) ? ((likely_depth(h) == 64) ? constant(value) : constant(float(value))) : constant(int(value), likely_depth(h)); }
    AllocaInst *autoAlloca(double value) const { AllocaInst *alloca = b->CreateAlloca(ty(), 0, n); b->CreateStore(autoConstant(value), alloca); return alloca; }

    Value *data(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 0), n+"_data"); }
    Value *data(Value *matrix, Type *type) const { return b->CreatePointerCast(data(matrix), type); }
    Value *channels(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 1), n+"_channels"); }
    Value *columns(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 2), n+"_columns"); }
    Value *rows(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 3), n+"_rows"); }
    Value *frames(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 4), n+"_frames"); }
    Value *hash(Value *matrix) const { return b->CreateLoad(b->CreateStructGEP(matrix, 5), n+"_hash"); }

    Value *data(bool cast = true) const { return cast ? data(v, ty(true)) : data(v); }
    Value *channels() const { return likely_is_single_channel(h) ? static_cast<Value*>(one()) : channels(v); }
    Value *columns() const { return likely_is_single_column(h) ? static_cast<Value*>(one()) : columns(v); }
    Value *rows() const { return likely_is_single_row(h) ? static_cast<Value*>(one()) : rows(v); }
    Value *frames() const { return likely_is_single_frame(h) ? static_cast<Value*>(one()) : frames(v); }
    Value *hash() const { return hash(v); }

    void setData(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 0)); }
    void setChannels(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 1)); }
    void setColumns(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 2)); }
    void setRows(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 3)); }
    void setFrames(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 4)); }
    void setHash(Value *matrix, Value *value) const { b->CreateStore(value, b->CreateStructGEP(matrix, 5)); }

    void setData(Value *value) const { setData(v, value); }
    void setChannels(Value *value) const { setChannels(v, value); }
    void setColumns(Value *value) const { setColumns(v, value); }
    void setRows(Value *value) const { setRows(v, value); }
    void setFrames(Value *value) const { setFrames(v, value); }
    void setHash(Value *value) const { setHash(v, value); }

    void copyHeaderTo(Value *matrix) const {
        setChannels(matrix, channels());
        setColumns(matrix, columns());
        setRows(matrix, rows());
        setFrames(matrix, frames());
        setHash(matrix, hash());
    }

    void deallocate() const {
        static Function *free = TheModule->getFunction("free");
        if (!free) {
            Type *freeReturn = Type::getVoidTy(getGlobalContext());
            vector<Type*> freeParams;
            freeParams.push_back(Type::getInt8PtrTy(getGlobalContext()));
            FunctionType* freeType = FunctionType::get(freeReturn, freeParams, false);
            free = Function::Create(freeType, GlobalValue::ExternalLinkage, "free", TheModule);
            free->setCallingConv(CallingConv::C);
        }

        vector<Value*> freeArgs;
        freeArgs.push_back(b->CreateStructGEP(v, 0));
        b->CreateCall(free, freeArgs);
        setData(ConstantPointerNull::get(Type::getInt8PtrTy(getGlobalContext())));
    }

    Value *get(int mask) const { return b->CreateAnd(hash(), constant(mask, 16)); }
    void set(int value, int mask) const { setHash(b->CreateOr(b->CreateAnd(hash(), constant(~mask, 16)), b->CreateAnd(constant(value, 16), constant(mask, 16)))); }
    void setBit(bool on, int mask) const { on ? setHash(b->CreateOr(hash(), constant(mask, 16))) : setHash(b->CreateAnd(hash(), constant(~mask, 16))); }

    Value *bits() const { return get(likely_hash_depth); }
    void setBits(int bits) const { set(bits, likely_hash_depth); }
    Value *isSigned() const { return get(likely_hash_signed); }
    void setSigned(bool isSigned) const { setBit(isSigned, likely_hash_signed); }
    Value *isFloating() const { return get(likely_hash_floating); }
    void setFloating(bool isFloating) const { if (isFloating) setSigned(true); setBit(isFloating, likely_hash_floating); }
    Value *type() const { return get(likely_hash_depth + likely_hash_floating + likely_hash_signed); }
    void setType(int type) const { set(type, likely_hash_depth + likely_hash_floating + likely_hash_signed); }
    Value *isParallel() const { return get(likely_hash_parallel); }
    void setParallel(bool isParallel) const { setBit(isParallel, likely_hash_parallel); }
    Value *isHeterogeneous() const { return get(likely_hash_heterogeneous); }
    void setHeterogeneous(bool isHeterogeneous) const { setBit(isHeterogeneous, likely_hash_heterogeneous); }
    Value *isSingleChannel() const { return get(likely_hash_single_channel); }
    void setSingleChannel(bool isSingleChannel) const { setBit(isSingleChannel, likely_hash_single_channel); }
    Value *isSingleColumn() const { return get(likely_hash_single_column); }
    void setSingleColumn(bool isSingleColumn) { setBit(isSingleColumn, likely_hash_single_column); }
    Value *isSingleRow() const { return get(likely_hash_single_row); }
    void setSingleRow(bool isSingleRow) const { setBit(isSingleRow, likely_hash_single_row); }
    Value *isSingleFrame() const { return get(likely_hash_single_frame); }
    void setSingleFrame(bool isSingleFrame) const { setBit(isSingleFrame, likely_hash_single_frame); }
    Value *elements() const { return b->CreateMul(b->CreateMul(b->CreateMul(channels(), columns()), rows()), frames()); }
    Value *bytes() const { return b->CreateMul(b->CreateUDiv(b->CreateCast(Instruction::ZExt, bits(), Type::getInt32Ty(getGlobalContext())), constant(8, 32)), elements()); }

    Value *columnStep() const { Value *columnStep = channels(); columnStep->setName(n+"_cStep"); return columnStep; }
    Value *rowStep() const { return b->CreateMul(columns(), columnStep(), n+"_rStep"); }
    Value *frameStep() const { return b->CreateMul(rows(), rowStep(), n+"_tStep"); }

    Value *index(Value *c) const { return likely_is_single_channel(h) ? constant(0) : c; }
    Value *index(Value *c, Value *x) const { return likely_is_single_column(h) ? index(c) : b->CreateAdd(b->CreateMul(x, columnStep()), index(c)); }
    Value *index(Value *c, Value *x, Value *y) const { return likely_is_single_row(h) ? index(c, x) : b->CreateAdd(b->CreateMul(y, rowStep()), index(c, x)); }
    Value *index(Value *c, Value *x, Value *y, Value *f) const { return likely_is_single_frame(h) ? index(c, x, y) : b->CreateAdd(b->CreateMul(f, frameStep()), index(c, x, y)); }

    void deindex(Value *i, Value **c) const {
        *c = likely_is_single_channel(h) ? constant(0) : i;
    }
    void deindex(Value *i, Value **c, Value **x) const {
        Value *rem;
        if (likely_is_single_column(h)) {
            rem = i;
            *x = constant(0);
        } else {
            Value *step = columnStep();
            rem = b->CreateURem(i, step, n+"_xRem");
            *x = b->CreateExactUDiv(b->CreateSub(i, rem), step, n+"_x");
        }
        deindex(rem, c);
    }
    void deindex(Value *i, Value **c, Value **x, Value **y) const {
        Value *rem;
        if (likely_is_single_row(h)) {
            rem = i;
            *y = constant(0);
        } else {
            Value *step = rowStep();
            rem = b->CreateURem(i, step, n+"_yRem");
            *y = b->CreateExactUDiv(b->CreateSub(i, rem), step, n+"_y");
        }
        deindex(rem, c, x);
    }
    void deindex(Value *i, Value **c, Value **x, Value **y, Value **t) const {
        Value *rem;
        if (likely_is_single_frame(h)) {
            rem = i;
            *t = constant(0);
        } else {
            Value *step = frameStep();
            rem = b->CreateURem(i, step, n+"_tRem");
            *t = b->CreateExactUDiv(b->CreateSub(i, rem), step, n+"_t");
        }
        deindex(rem, c, x, y);
    }

    LoadInst *load(Value *matrix, Type *type, Value *i) {
        LoadInst *load = b->CreateLoad(b->CreateGEP(data(matrix, type), i));
        load->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return load;
    }
    LoadInst *load(Value *i) {
        LoadInst *load = b->CreateLoad(b->CreateGEP(data(), i));
        load->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return load;
    }

    StoreInst *store(Value *matrix, Value *i, Value *value) {
        Value *d = data(matrix, ty(true));
        Value *idx = b->CreateGEP(d, i);
        StoreInst *store = b->CreateStore(value, idx);
        store->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return store;
    }
    StoreInst *store(Value *i, Value *value) {
        StoreInst *store = b->CreateStore(value, b->CreateGEP(data(), i));
        store->setMetadata("llvm.mem.parallel_loop_access", getCurrentNode());
        return store;
    }

    Value *cast(Value *i, const MatrixBuilder &dst) const { return (likely_type(h) == likely_type(dst.h)) ? i : b->CreateCast(CastInst::getCastOpcode(i, likely_is_signed(h), dst.ty(), likely_is_signed(h)), i, dst.ty()); }
    Value *add(Value *i, Value *j) const { return likely_is_floating(h) ? b->CreateFAdd(i, j, n) : b->CreateAdd(i, j, n); }
    Value *subtract(Value *i, Value *j) const { return likely_is_floating(h) ? b->CreateFSub(i, j, n) : b->CreateSub(i, j, n); }
    Value *multiply(Value *i, Value *j) const { return likely_is_floating(h) ? b->CreateFMul(i, j, n) : b->CreateMul(i, j, n); }
    Value *divide(Value *i, Value *j) const { return likely_is_floating(h) ? b->CreateFDiv(i, j, n) : (likely_is_signed(h) ? b->CreateSDiv(i,j, n) : b->CreateUDiv(i, j, n)); }

    Value *compareLT(Value *i, Value *j) const { return likely_is_floating(h) ? b->CreateFCmpOLT(i, j) : (likely_is_signed(h) ? b->CreateICmpSLT(i, j) : b->CreateICmpULT(i, j)); }
    Value *compareGT(Value *i, Value *j) const { return likely_is_floating(h) ? b->CreateFCmpOGT(i, j) : (likely_is_signed(h) ? b->CreateICmpSGT(i, j) : b->CreateICmpUGT(i, j)); }

    Loop beginLoop(BasicBlock *entry, Value *start, Value *stop) {
        Loop loop;
        loop.stop = stop;
        loop.body = BasicBlock::Create(getGlobalContext(), n+"_loop_body", f);
        loop.exit = BasicBlock::Create(getGlobalContext(), n+"_loop_exit", f);

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

        loop.i = b->CreatePHI(Type::getInt32Ty(getGlobalContext()), 2, n);
        loop.i->addIncoming(start, entry);

        loops.push(loop);
        return loop;
    }

    void endLoop() {
        const Loop &loop = loops.top();
        Value *increment = b->CreateAdd(loop.i, one(), n+"_loop_increment");
        loop.i->addIncoming(increment, loop.body);

        BranchInst *latch = b->CreateCondBr(b->CreateICmpEQ(increment, loop.stop, n+"_loop_test"), loop.exit, loop.body);
        latch->setMetadata("llvm.loop.parallel", loop.node);

        b->SetInsertPoint(loop.exit);
        loops.pop();
    }

    MDNode *getCurrentNode() const { return loops.empty() ? NULL : loops.top().node; }

    template <typename T>
    inline static vector<T> toVector(T value) { vector<T> vector; vector.push_back(value); return vector; }

    static Type *ty(likely_hash hash, bool pointer = false)
    {
        const int bits = likely_depth(hash);
        const bool floating = likely_is_floating(hash);
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
    inline Type *ty(bool pointer = false) const { return ty(h, pointer); }
    inline vector<Type*> tys(bool pointer = false) const { return toVector<Type*>(ty(pointer)); }  
};

class KernelBuilder
{
    likely_description description;
    Definition definition;
    vector<string> arguments;
    vector<likely_hash> hashes;
    MatrixBuilder kernel;
    PHINode *i;

public:
    KernelBuilder() : i(NULL) {}
    KernelBuilder(likely_description description_)
        : description(description_), i(NULL)
    {
        const string str(description);
        const size_t lParen = str.find('(');
        const string name = str.substr(0, lParen);
        arguments = split(str.substr(lParen+1, str.size()-lParen-2), ',');
        definition = Definition::get(name);
        likely_assert(arguments.size() == definition.parameters.size(), "KernelBuilder::KernelBuilder function: %s requires: %zu parameters but was only given: %zu arguments", name.c_str(), definition.parameters.size(), arguments.size());
    }

    void makeAllocation(Function *function, const vector<likely_hash> &hashes_)
    {
        hashes = hashes_;
        vector<Value*> srcs;
        Value *dst;
        getValues(function, srcs, dst);

        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);

        likely_hash kernelHash = likely_hash_null;
        for (likely_hash hash : hashes)
            kernelHash |= hash;

        kernel = MatrixBuilder(&builder, function, "kernel", kernelHash, srcs[0]);
        kernel.copyHeaderTo(dst);
        builder.CreateRet(kernel.elements());
    }

    void makeKernel(Function *function)
    {
        vector<Value*> srcs;
        Value *dst, *start, *stop;
        getValues(function, srcs, dst, start, stop);

        function->addFnAttr(Attribute::NoUnwind);
        for (size_t i=1; i<function->arg_size()-1; i++) { // Exclude kernel start and stop arguments
            function->setDoesNotAlias(i);
            function->setDoesNotCapture(i);
        }

        BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
        IRBuilder<> builder(entry);

        if (likely_is_parallel(kernel.h)) {
            vector<likely_matrix> matricies;
            for (likely_hash hash : hashes)
                matricies.push_back(Matrix(hash).setParallel(false));

            void *serialKernelFunction = NULL;
            switch (matricies.size()) {
              case 0: serialKernelFunction = likely_make_kernel(description, 0, NULL); break;
              case 1: serialKernelFunction = likely_make_kernel(description, 1, &matricies[0], NULL); break;
              case 2: serialKernelFunction = likely_make_kernel(description, 2, &matricies[0], &matricies[1], NULL); break;
              case 3: serialKernelFunction = likely_make_kernel(description, 3, &matricies[0], &matricies[1], &matricies[2], NULL); break;
              default: likely_assert(false, "KernelBuilder::make kernel invalid arity: %zu", matricies.size());
            }

            static Function *parallelDispatch = NULL;
            if (parallelDispatch == NULL) {
                vector<Type*> params;
                params.push_back(Type::getInt8PtrTy(getGlobalContext()));
                params.push_back(Type::getInt8Ty(getGlobalContext()));
                params.push_back(Type::getInt32Ty(getGlobalContext()));
                params.push_back(Type::getInt32Ty(getGlobalContext()));
                params.push_back(PointerType::getUnqual(TheMatrixStruct));
                parallelDispatch = Function::Create(FunctionType::get(Type::getVoidTy(getGlobalContext()), params, true), GlobalValue::ExternalLinkage, "likely_parallel_dispatch", TheModule);
                parallelDispatch->setCallingConv(CallingConv::C);
            }

            vector<Value*> args;
            args.push_back(MatrixBuilder::constant(reinterpret_cast<const char*>(serialKernelFunction)));
            args.push_back(MatrixBuilder::constant(matricies.size(), 8));
            args.push_back(start);
            args.push_back(stop);
            args.insert(args.end(), srcs.begin(), srcs.end());
            args.push_back(dst);

            builder.CreateCall(parallelDispatch, args);
        } else {
            kernel.reset(&builder, function, srcs[0]);
            i = kernel.beginLoop(entry, start, stop).i;
            kernel.store(dst, i, makeEquation(definition.equation));
            kernel.endLoop();
        }

        builder.CreateRetVoid();
    }

    static void getValues(Function *function, vector<Value*> &srcs, Value *&dst)
    {
        Function::arg_iterator args = function->arg_begin();
        int i = 0;
        while (args != function->arg_end()) {
            Value *src = args++;
            stringstream name; name << "src" << char(int('A')+(i++));
            src->setName(name.str());
            srcs.push_back(src);
        }
        dst = srcs.back();
        srcs.pop_back();
        dst->setName("dst");
    }

    static void getValues(Function *function, vector<Value*> &srcs, Value *&dst, Value *&start, Value *&stop)
    {
        getValues(function, srcs, dst);
        stop = dst;
        stop->setName("stop");
        start = srcs.back(); srcs.pop_back();
        start->setName("start");
        dst = srcs.back(); srcs.pop_back();
        dst->setName("dst");
    }

    Value *makeEquation(const Node &node)
    {
        if      (node.value == "+") return kernel.add(makeEquation(node.children[0]), makeEquation(node.children[1]));
        else if (node.value == "-") return kernel.subtract(makeEquation(node.children[0]), makeEquation(node.children[1]));
        else                        return makeTerm(node);
    }

    Value *makeTerm(const Node &node)
    {
        if      (node.value == "*") return kernel.multiply(makeEquation(node.children[0]), makeEquation(node.children[1]));
        else if (node.value == "/") return kernel.divide(makeEquation(node.children[0]), makeEquation(node.children[1]));
        else                        return makeFactor(node);
    }

    Value *makeFactor(const Node &node)
    {
        Value *            value = makeMatrix(node);
        if (value == NULL) value = makeParameter(node);
        if (value == NULL) value = makeNumber(node);
        likely_assert(value != NULL, "KernelBuilder::makeFactor code generation failed for factor: %s", node.value.c_str());
        return value;
    }

    Value *makeMatrix(const Node &node)
    {
        if (node.value == "src") return kernel.load(i);
        else                     return NULL;
    }

    Value *makeParameter(const Node &node)
    {
        const int index = indexOf(definition.parameters, node.value);
        if (index != -1) return makeNumber(Node(arguments[index], node.children));
        else             return NULL;
    }

    Value *makeNumber(const Node &node)
    {
        char *endptr;
        double value = strtod(node.value.c_str(), &endptr);
        if (*endptr) return NULL;
        else         return kernel.autoConstant(value);
    }
};

static vector<likely_description> descriptions;
static map<string,KernelBuilder> kernels;
static ExecutionEngine *executionEngine = NULL;
static TargetMachine *targetMachine = NULL;
static recursive_mutex makerLock;

// Control parallel execution
static vector<mutex*> workers;
static mutex workersActive;
static atomic_uint workersRemaining(0);
static void *workerKernel = NULL;
static likely_arity workerArity = 0;
static likely_size workerStart = 0;
static likely_size workerStop = 0;
static likely_matrix *workerMatricies[LIKELY_NUM_ARITIES+1];

static string mangledName(const string &description, const vector<likely_hash> &hashes)
{
    stringstream stream; stream << description;
    for (likely_hash hash : hashes)
        stream << "_" << hex << setfill('0') << setw(4) << hash;
    return stream.str();
}

static Function *getFunction(const string &description, likely_arity arity, Type *ret, Type *start = NULL, Type *stop = NULL)
{
    PointerType *matrixPointer = PointerType::getUnqual(TheMatrixStruct);
    Function *function;
    switch (arity) {
      case 0: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, start, stop, NULL)); break;
      case 1: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, matrixPointer,  start, stop, NULL)); break;
      case 2: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, matrixPointer, matrixPointer,  start, stop, NULL)); break;
      case 3: function = cast<Function>(TheModule->getOrInsertFunction(description, ret, matrixPointer, matrixPointer, matrixPointer, matrixPointer,  start, stop, NULL)); break;
      default: likely_assert(false, "FunctionBuilder::getFunction invalid arity: %d", arity);
    }
    function->setCallingConv(CallingConv::C);
    return function;
}

static void executeWorker(int workerID)
{
    // There are hardware_concurrency-1 helper threads
    // The main thread which assumes workerID = workers.size()
    const likely_size step = 1 + (workerStop - workerStart - 1)/(workers.size()+1);
    const likely_size start = workerStart + workerID * step;
    const likely_size stop = std::min(workerStart + (workerID+1)*step, workerStop);

    switch (workerArity) {
      case 0: reinterpret_cast<likely_nullary_kernel>(workerKernel)(workerMatricies[0], start, stop); break;
      case 1: reinterpret_cast<likely_unary_kernel>(workerKernel)(workerMatricies[0], workerMatricies[1], start, stop); break;
      case 2: reinterpret_cast<likely_binary_kernel>(workerKernel)(workerMatricies[0], workerMatricies[1], workerMatricies[2], start, stop); break;
      case 3: reinterpret_cast<likely_ternary_kernel>(workerKernel)(workerMatricies[0], workerMatricies[1], workerMatricies[2], workerMatricies[3], start, stop); break;
      default: likely_assert(false, "likely_parallel_dispatch invalid arity: %d", workerArity);
    }
}

static void workerThread(int workerID)
{
    while (true) {
        workers[workerID]->lock();
        executeWorker(workerID);
        if (--workersRemaining == 0)
            workersActive.unlock();
    }
}

} // namespace likely

using namespace likely;

void *likely_make_function(likely_description description, likely_arity arity)
{
    lock_guard<recursive_mutex> lock(makerLock);

    if (TheModule == NULL) {
        // Initialize Likely
        InitializeNativeTarget();
        TheModule = new Module("likely", getGlobalContext());
        TheModule->setTargetTriple(sys::getProcessTriple());

        string error;
        EngineBuilder engineBuilder(TheModule);
        engineBuilder.setMCPU(sys::getHostCPUName());
        engineBuilder.setEngineKind(EngineKind::JIT);
        engineBuilder.setOptLevel(CodeGenOpt::Aggressive);
        engineBuilder.setErrorStr(&error);

        executionEngine = engineBuilder.create();
        likely_assert(executionEngine != NULL, "likely_make_function failed to create LLVM ExecutionEngine with error: %s", error.c_str());

        targetMachine = engineBuilder.selectTarget();
        likely_assert(targetMachine != NULL, "likely_make_function failed to create LLVM TargetMachine with error: %s", error.c_str());

        TheMatrixStruct = StructType::create("Matrix",
                                             Type::getInt8PtrTy(getGlobalContext()), // data
                                             Type::getInt32Ty(getGlobalContext()),   // channels
                                             Type::getInt32Ty(getGlobalContext()),   // columns
                                             Type::getInt32Ty(getGlobalContext()),   // rows
                                             Type::getInt32Ty(getGlobalContext()),   // frames
                                             Type::getInt16Ty(getGlobalContext()),   // hash
                                             NULL);

        const int numWorkers = std::max((int)thread::hardware_concurrency()-1, 1);
        for (int i=0; i<numWorkers; i++) {
            mutex *m = new mutex();
            m->lock();
            workers.push_back(m);
            thread(workerThread, i).detach();
        }
    }

    Function *function = TheModule->getFunction(description);
    if (function != NULL)
        return executionEngine->getPointerToFunction(function);
    function = getFunction(description, arity, Type::getVoidTy(getGlobalContext()));

    static vector<Type*> makerParameters;
    if (makerParameters.empty()) {
        makerParameters.push_back(Type::getInt8PtrTy(getGlobalContext()));
        makerParameters.push_back(Type::getInt8Ty(getGlobalContext()));
        makerParameters.push_back(PointerType::getUnqual(TheMatrixStruct));
    }

    static Function *makeAllocationFunction = NULL;
    static vector<PointerType*> allocationFunctionTypes(LIKELY_NUM_ARITIES, NULL);
    PointerType *allocationFunctionType = allocationFunctionTypes[arity];
    if (allocationFunctionType == NULL) {
        vector<Type*> allocationParams;
        for (int i=0; i<arity+1; i++)
            allocationParams.push_back(PointerType::getUnqual(TheMatrixStruct));
        Type *allocationReturn = Type::getInt32Ty(getGlobalContext());
        allocationFunctionType = PointerType::getUnqual(FunctionType::get(allocationReturn, allocationParams, false));
        allocationFunctionTypes[arity] = allocationFunctionType;

        if (makeAllocationFunction == NULL) {
            FunctionType* makeAllocationType = FunctionType::get(allocationFunctionType, makerParameters, true);
            makeAllocationFunction = Function::Create(makeAllocationType, GlobalValue::ExternalLinkage, "likely_make_allocation", TheModule);
            makeAllocationFunction->setCallingConv(CallingConv::C);
        }
    }
    GlobalVariable *allocationFunction = cast<GlobalVariable>(TheModule->getOrInsertGlobal(string(description)+"_allocation", allocationFunctionType));
    allocationFunction->setInitializer(ConstantPointerNull::get(allocationFunctionType));

    static Function *makeKernelFunction = NULL;
    static vector<PointerType*> kernelFunctionTypes(LIKELY_NUM_ARITIES, NULL);
    PointerType *kernelFunctionType = kernelFunctionTypes[arity];
    if (kernelFunctionType == NULL) {
        vector<Type*> kernelParams;
        for (int i=0; i<arity+1; i++)
            kernelParams.push_back(PointerType::getUnqual(TheMatrixStruct));
        kernelParams.push_back(Type::getInt32Ty(getGlobalContext()));
        kernelParams.push_back(Type::getInt32Ty(getGlobalContext()));
        Type *kernelReturn = Type::getVoidTy(getGlobalContext());
        kernelFunctionType = PointerType::getUnqual(FunctionType::get(kernelReturn, kernelParams, false));
        kernelFunctionTypes[arity] = kernelFunctionType;

        if (makeKernelFunction == NULL) {
            FunctionType* makeUnaryKernelType = FunctionType::get(kernelFunctionType, makerParameters, true);
            makeKernelFunction = Function::Create(makeUnaryKernelType, GlobalValue::ExternalLinkage, "likely_make_kernel", TheModule);
            makeKernelFunction->setCallingConv(CallingConv::C);
        }
    }
    GlobalVariable *kernelFunction = cast<GlobalVariable>(TheModule->getOrInsertGlobal(string(description)+"_kernel", kernelFunctionType));
    kernelFunction->setInitializer(ConstantPointerNull::get(kernelFunctionType));

    vector<Value*> srcs;
    Value *dst;
    KernelBuilder::getValues(function, srcs, dst);

    BasicBlock *entry = BasicBlock::Create(getGlobalContext(), "entry", function);
    IRBuilder<> builder(entry);

    vector<GlobalVariable*> kernelHashes;
    for (int i=0; i<arity; i++) {
        GlobalVariable *kernelHash = cast<GlobalVariable>(TheModule->getOrInsertGlobal(string(description)+"_hash"+to_string(i), Type::getInt16Ty(getGlobalContext())));
        kernelHash->setInitializer(MatrixBuilder::constant(0, 16));
        kernelHashes.push_back(kernelHash);
    }

    vector<Value*> srcHashes;
    for (int i=0; i<arity; i++)
        srcHashes.push_back(builder.CreateLoad(builder.CreateStructGEP(srcs[i], 5), "src_hash"+to_string(i)));

    Value *hashTest = MatrixBuilder::constant(true);
    for (int i=0; i<arity; i++)
        hashTest = builder.CreateAnd(hashTest, builder.CreateICmpEQ(builder.CreateLoad(kernelHashes[i]), srcHashes[i]));

    BasicBlock *hashFail = BasicBlock::Create(getGlobalContext(), "hash_fail", function);
    BasicBlock *execute = BasicBlock::Create(getGlobalContext(), "execute", function);
    builder.CreateCondBr(hashTest, execute, hashFail);

    builder.SetInsertPoint(hashFail);
    {
        // Construct a description that stays valid for the lifetime of the program
        char *copy = new char[strlen(description)+1];
        strcpy(copy, description);
        descriptions.push_back(copy);

        vector<Value*> args;
        args.push_back(MatrixBuilder::constant(copy));
        args.push_back(MatrixBuilder::constant(arity, 8));
        args.insert(args.end(), srcs.begin(), srcs.end());
        builder.CreateStore(builder.CreateCall(makeAllocationFunction, args), allocationFunction);
        builder.CreateStore(builder.CreateCall(makeKernelFunction, args), kernelFunction);
        for (int i=0; i<arity; i++)
            builder.CreateStore(srcHashes[i], kernelHashes[i]);
        builder.CreateBr(execute);
    }

    builder.SetInsertPoint(execute);
    {
        vector<Value*> args(srcs); args.push_back(dst);
        Value *kernelSize = builder.CreateCall(builder.CreateLoad(allocationFunction), args);

        static Function *likely_allocate = NULL;
        if (likely_allocate == NULL) {
            Type *allocateReturn = Type::getVoidTy(getGlobalContext());
            vector<Type*> allocateParameters;
            allocateParameters.push_back(PointerType::getUnqual(TheMatrixStruct));
            FunctionType* allocateType = FunctionType::get(allocateReturn, allocateParameters, false);
            likely_allocate = Function::Create(allocateType, GlobalValue::ExternalLinkage, "likely_allocate", TheModule);
            likely_allocate->setCallingConv(CallingConv::C);
        }
        builder.CreateCall(likely_allocate, dst);

        BasicBlock *kernel = BasicBlock::Create(getGlobalContext(), "kernel", function);
        BasicBlock *exit = BasicBlock::Create(getGlobalContext(), "exit", function);
        builder.CreateCondBr(builder.CreateICmpUGT(kernelSize, MatrixBuilder::zero()), kernel, exit);

        builder.SetInsertPoint(kernel);
        args.push_back(MatrixBuilder::zero());
        args.push_back(kernelSize);
        builder.CreateCall(builder.CreateLoad(kernelFunction), args);
        builder.CreateBr(exit);

        builder.SetInsertPoint(exit);
        builder.CreateRetVoid();
    }

    static FunctionPassManager *functionPassManager = NULL;
    if (functionPassManager == NULL) {
        functionPassManager = new FunctionPassManager(TheModule);
        functionPassManager->add(createVerifierPass(PrintMessageAction));
    }
    functionPassManager->run(*function);

    return executionEngine->getPointerToFunction(function);
}

void *likely_make_allocation(likely_description description, likely_arity arity, likely_matrix *src, ...)
{
    vector<likely_hash> hashes;
    va_list ap;
    va_start(ap, src);
    for (int i=0; i<arity; i++) {
        hashes.push_back(src->hash);
        src = va_arg(ap, likely_matrix*);
    }
    va_end(ap);

    lock_guard<recursive_mutex> lock(makerLock);
    const string name = mangledName(description, hashes)+"_allocation";

    Function *function = TheModule->getFunction(name);
    if (function != NULL)
        return executionEngine->getPointerToFunction(function);
    function = getFunction(name, hashes.size(), Type::getInt32Ty(getGlobalContext()));

    auto kernelPointer = kernels.find(description);
    if (kernelPointer == kernels.end()) {
        kernels[description] = KernelBuilder(description);
        kernelPointer = kernels.find(description);
    }
    (*kernelPointer).second.makeAllocation(function, hashes);

    static FunctionPassManager *functionPassManager = NULL;
    if (functionPassManager == NULL) {
        functionPassManager = new FunctionPassManager(TheModule);
        functionPassManager->add(createVerifierPass(PrintMessageAction));
    }
    functionPassManager->run(*function);

    return executionEngine->getPointerToFunction(function);
}

void *likely_make_kernel(likely_description description, likely_arity arity, likely_matrix *src, ...)
{
    vector<likely_hash> hashes;
    va_list ap;
    va_start(ap, src);
    for (int i=0; i<arity; i++) {
        hashes.push_back(src->hash);
        src = va_arg(ap, likely_matrix*);
    }
    va_end(ap);

    lock_guard<recursive_mutex> lock(makerLock);
    const string name = mangledName(description, hashes)+"_kernel";

    Function *function = TheModule->getFunction(name);
    if (function != NULL)
        return executionEngine->getPointerToFunction(function);
    function = getFunction(name, hashes.size(), Type::getVoidTy(getGlobalContext()), Type::getInt32Ty(getGlobalContext()), Type::getInt32Ty(getGlobalContext()));

    kernels[description].makeKernel(function);

    static FunctionPassManager *functionPassManager = NULL;
    if (functionPassManager == NULL) {
        PassRegistry &registry = *PassRegistry::getPassRegistry();
        initializeScalarOpts(registry);

        functionPassManager = new FunctionPassManager(TheModule);
        functionPassManager->add(createVerifierPass(PrintMessageAction));
        targetMachine->addAnalysisPasses(*functionPassManager);
        functionPassManager->add(new TargetLibraryInfo(Triple(TheModule->getTargetTriple())));
        functionPassManager->add(new DataLayout(TheModule));
        functionPassManager->add(createBasicAliasAnalysisPass());
        functionPassManager->add(createLICMPass());
//        functionPassManager->add(createCFGSimplificationPass());
//        functionPassManager->add(createEarlyCSEPass());
//        functionPassManager->add(createInstructionCombiningPass());
//        functionPassManager->add(createDeadCodeEliminationPass());
//        functionPassManager->add(createGVNPass());
//        functionPassManager->add(createDeadInstEliminationPass());
        functionPassManager->add(createLoopVectorizePass());
//        functionPassManager->add(createLoopUnrollPass(INT_MAX,8));
//        functionPassManager->add(createPrintFunctionPass("--------------------------------------------------------------------------------", &errs()));
//        DebugFlag = true;
    }
    functionPassManager->run(*function);

    return executionEngine->getPointerToFunction(function);
}

void likely_parallel_dispatch(void *kernel, likely_arity arity, likely_size start, likely_size stop, likely_matrix *src, ...)
{
    workersActive.lock();

    workersRemaining = workers.size();
    workerKernel = kernel;
    workerArity = arity;
    workerStart = start;
    workerStop = stop;

    va_list ap;
    va_start(ap, src);
    for (int i=0; i<arity+1; i++) {
        workerMatricies[i] = src;
        src = va_arg(ap, likely_matrix*);
    }
    va_end(ap);

    for (size_t i=0; i<workers.size(); i++)
        workers[i]->unlock();

    executeWorker(workers.size());
    while (workersRemaining > 0) {}
}
