// The SHARC language: its AST. Assembly instructions (compute, flow control,
// register moves, misc) keep their matched text -- registers and opcodes have
// no cross-references to resolve -- while declarations, directives and value
// expressions get real structure.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sharc::ast {

struct ValueExpr : cursiv::Extends<cursiv::AstNode> {};

struct NumberLit : cursiv::Extends<ValueExpr> {
  string_view value;
};

struct NameRef : cursiv::Extends<ValueExpr> {
  string_view name;
};

struct AddrOf : cursiv::Extends<ValueExpr> {
  string_view name;
};

struct LengthOf : cursiv::Extends<ValueExpr> {
  string_view name;
};

struct CharLit : cursiv::Extends<ValueExpr> {
  string_view value;
};

struct StringLit : cursiv::Extends<ValueExpr> {
  string_view value;
};

struct UnaryMinus : cursiv::Extends<ValueExpr> {
  ValueExpr* operand;
};

struct BitNot : cursiv::Extends<ValueExpr> {
  ValueExpr* operand;
};

enum class Operator : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  modulo,
  bit_or,
  bit_xor,
  bit_and,
  shift_left,
  shift_right,
};

struct BinaryExpr : cursiv::Extends<ValueExpr> {
  ValueExpr* left;
  Operator op;
  ValueExpr* right;
};

struct StmtAtom : cursiv::Extends<cursiv::AstNode> {};

struct SectionDecl : cursiv::Extends<StmtAtom> {
  string_view qualifier;
  string_view name;
};

struct SegmentDecl : cursiv::Extends<StmtAtom> {
  string_view qualifier;
  string_view name;
};

struct EndSeg : cursiv::Extends<StmtAtom> {};

// The five `.var` shapes fold into one class: a plain name list, a value
// list with no names, a scalar or array with an optional initializer.
struct Declaration : cursiv::Extends<StmtAtom> {
  vector<string_view> names;
  vector<ValueExpr*> values;
  boolean is_array;
  ValueExpr* size;
};

struct FileAttribute : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;  // raw symbol/string; empty means the native default, 1
};

struct Directive : cursiv::Extends<StmtAtom> {
  string_view keyword;
  vector<string_view> names;
  vector<string_view> strings;
  vector<ValueExpr*> values;
  vector<FileAttribute*> attributes;
};

// Instruction bodies: registers and opcodes have no cross-references or
// nested structure here. Payloads join original tokens with spaces; exact
// whitespace and comments remain in the CST.
struct Compute : cursiv::Extends<StmtAtom> {
  string_view raw;
};

struct FlowControl : cursiv::Extends<StmtAtom> {
  string_view raw;
};

struct ImmMov : cursiv::Extends<StmtAtom> {
  string_view raw;
};

struct Misc : cursiv::Extends<StmtAtom> {
  string_view raw;
};

struct IfComputeMov : cursiv::Extends<StmtAtom> {
  string_view raw;
};

struct ComputeMovExpr : cursiv::Extends<StmtAtom> {
  string_view raw;
};

struct Statement : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> labels;
  StmtAtom* atom;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

}  // namespace sharc::ast
