// The scotty language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace scotty::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// A function's own parameter -- the other name a bare identifier can target,
// besides a VarAssign.
struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct VarAssign : cursiv::Extends<Statement> {
  string_view name;
  Expr* value;
};

struct FnDef : cursiv::Extends<Statement> {
  string_view name;
  Param* param;
  Expr* body;
};

enum class Operator : std::uint8_t { add, subtract, multiply, divide };

// Prefix notation: the operator names both operands, never infix.
struct BinaryExpr : cursiv::Extends<Expr> {
  Operator op;
  Expr* left;
  Expr* right;
};

// `(f x)` -- func always targets a FnDef, never a Param: the grammar has no
// zero-argument call syntax.
struct Call : cursiv::Extends<Expr> {
  reference<FnDef> func;
  Expr* arg;
};

struct NameExpr : cursiv::Extends<Expr> {
  reference<VarAssign, Param> target;
};

struct NumberLiteral : cursiv::Extends<Expr> {
  boolean negative;
  string_view value;
};

struct Model : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
  Expr* result;
};

}  // namespace scotty::ast
