// The tinyc language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tinyc::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view value;
};

// `x = v`. The target is a bare name, never a general expression: the
// grammar admits no other lvalue.
struct Assign : cursiv::Extends<Expr> {
  Ident *target;
  Expr *value;
};

// `+`, `-`, and the single, non-chaining `<`: one class for all three, `op`
// holding which.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  string_view op;
  Expr *right;
};

// `expr;` and the empty statement `;`, which is why the expression is
// optional rather than a null pointer with a comment.
struct ExprStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct BlockStmt : cursiv::Extends<Stmt> {
  vector<Stmt *> statements;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr *cond;
  Stmt *thenBranch;
  Stmt *elseBranch;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr *cond;
  Stmt *body;
};

struct DoStmt : cursiv::Extends<Stmt> {
  Stmt *body;
  Expr *cond;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt *> statements;
};

} // namespace tinyc::ast
