// The redcode language: its AST. A file is a list of instructions and
// standalone comment lines, in source order.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace redcode::ast {

enum class Opcode : std::uint8_t {
  dat,
  mov,
  add,
  sub,
  mul,
  div,
  mod,
  jmp,
  jmz,
  jmn,
  djn,
  cmp,
  slt,
  spl,
  org,
  djz,
  seq,
  sne,
  nop,
  ldp,
  stp,
  end,
};

// `none` is the zero value: absent, an instruction with no `.modifier`.
enum class Modifier : std::uint8_t { none, a, b, ab, ba, f, x, i };

// `none` is the zero value: an operand written with no addressing symbol.
enum class Mode : std::uint8_t {
  none,
  immediate,
  direct,
  indirect,
  predecrement,
  postincrement,
  a_indirect,
  a_predecrement,
  a_postincrement,
};

struct Comment : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

enum class Op : std::uint8_t { add, sub, mul, div, mod, eq, ne, lt, le, gt, ge,
                              logical_and, logical_or, logical_not };

struct Number : cursiv::Extends<Expr> {
  bool negative;
  string_view magnitude;  // exact unsigned digits, without layout
};

struct Unary : cursiv::Extends<Expr> {
  Op op;
  Expr* operand;
};

struct Paren : cursiv::Extends<Expr> {
  Expr* inner;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Op op;
  Expr* right;
};

struct Field : cursiv::Extends<cursiv::AstNode> {
  Mode mode;
  Expr* value;
};

struct Instruction : cursiv::Extends<cursiv::AstNode> {
  Opcode opcode;
  Modifier modifier;
  Field* field_a;
  Field* field_b;  // nullptr: no second operand
  Comment* comment;  // nullptr: no trailing comment
};

struct File : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<Instruction*, Comment*>> lines;
};

}  // namespace redcode::ast
