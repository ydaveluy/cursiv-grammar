#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace microc::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view value;
};

enum class BinOp : std::uint8_t { less, add, subtract };

// Shared by the sum ladder and the single comparison below it: `op` tells
// them apart, not the node type.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

struct Assign : cursiv::Extends<Expr> {
  Ident* target;
  Expr* value;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct Block : cursiv::Extends<Stmt> {
  vector<Stmt*> statements;
};

struct If : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct While : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> statements;
};

}  // namespace microc::ast
