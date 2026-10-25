// The corundum language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace corundum::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<Stmt> {};

struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> statements;
};

struct IntLit : cursiv::Extends<Expr> {
  int64 value;
};

struct FloatLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NilLit : cursiv::Extends<Expr> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct GlobalIdent : cursiv::Extends<Expr> {
  string_view name;
};

struct Index : cursiv::Extends<Expr> {
  Expr* base;
  Expr* index;
};

struct ArrayLit : cursiv::Extends<Expr> {
  vector<Expr*> elements;
};

struct NamedArg : cursiv::Extends<Expr> {
  string_view name;
  Expr* value;
};

struct Call : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
};

enum class BinOp : std::uint8_t {
  add,
  sub,
  mul,
  div,
  mod,
  exp,
  shl,
  shr,
  bit_and,
  bit_or,
  bit_xor,
  lt,
  gt,
  le,
  ge,
  eq,
  ne,
  logical_and,
  logical_or,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

enum class UnOp : std::uint8_t { logical_not, bit_not };

struct UnaryExpr : cursiv::Extends<Expr> {
  UnOp op;
  Expr* operand;
};

enum class AssignOp : std::uint8_t { assign, add, sub, mul, div, mod, exp };

struct Assign : cursiv::Extends<Expr> {
  Expr* target;
  AssignOp op;
  Expr* value;
};

struct ElsIf : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  Block* body;
};

// One class for `if` and `unless`: `negated` is the only difference the
// grammar admits between them.
struct Conditional : cursiv::Extends<Stmt> {
  boolean negated;
  Expr* cond;
  Block* thenBody;
  vector<ElsIf*> elifs;
  Block* elseBody;
};

struct While : cursiv::Extends<Stmt> {
  Expr* cond;
  Block* body;
};

struct For : cursiv::Extends<Stmt> {
  vector<Assign*> init;
  Expr* cond;
  vector<Assign*> step;
  Block* body;
};

struct Return : cursiv::Extends<Stmt> {
  Expr* value;
};

struct Require : cursiv::Extends<Stmt> {
  StringLit* value;
};

struct Pir : cursiv::Extends<Stmt> {
  Block* body;
};

struct Retry : cursiv::Extends<Stmt> {};
struct Break : cursiv::Extends<Stmt> {};

struct FuncDef : cursiv::Extends<Stmt> {
  string_view name;
  vector<string_view> params;
  Block* body;
};

}  // namespace corundum::ast
