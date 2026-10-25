// The tiny language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tiny::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

enum class Op : std::uint8_t { add, subtract };

struct VarRef : cursiv::Extends<Expr> {
  string_view name;
};

struct IntegerLiteral : cursiv::Extends<Expr> {
  string_view value;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Op op;
  Expr* right;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  string_view target;
  Expr* value;
};

struct ReadStmt : cursiv::Extends<Stmt> {
  vector<string_view> names;
};

struct WriteStmt : cursiv::Extends<Stmt> {
  vector<Expr*> values;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> statements;
};

}  // namespace tiny::ast
