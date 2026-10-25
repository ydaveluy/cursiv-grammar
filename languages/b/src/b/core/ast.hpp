// The b language: its AST. B has one flat, typeless namespace per
// function body (names, labels, extrn/auto declarations all share it), so
// every name here is a bare `string_view` -- no `reference<>` resolves
// across that namespace.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace b::ast {

// Base for anything a statement list may hold.
struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<Item> {};

// An expression, empty for the same reason as `Stmt`: any leaf below fits
// any operand slot through the family check against this base.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view value;
};

// Text after decoding B's asterisk escapes.
struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

// `'...'`: a B character constant -- a packed word, not a pointer, so its
// own class rather than a flag on `StringLit`.
struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

// `-x`, `!x`, `++x`, `--x`.
struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};

// `x++`, `x--`. A separate class from the prefix form: the two compute
// different values.
struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};

// `&x`: a name, or any other lvalue (`&*p`, `&a[i]`).
struct AddrExpr : cursiv::Extends<Expr> {
  Expr *target;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<Expr *> args;
};

// `array[index]`: interpreted as `*(array+index)` (Users' Reference to B's
// primary expressions), kept as its own shape since the two operands are
// each individually meaningful.
struct IndexExpr : cursiv::Extends<Expr> {
  Expr *array;
  Expr *index;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  string_view op;
  Expr *right;
};

// `c ? a : b`.
struct CondExpr : cursiv::Extends<Expr> {
  Expr *cond;
  Expr *thenValue;
  Expr *elseValue;
};

// `x = v`. `op` is empty for a plain `=`, or the one binary-operator
// spelling of a compound form (`x =+ v` is `x = x + v`). `target` is a
// name, an indirection (`*p = v`) or an index (`a[i] = v`).
struct AssignExpr : cursiv::Extends<Expr> {
  Expr *target;
  string_view op;
  Expr *value;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct NullStmt : cursiv::Extends<Stmt> {};

struct Block : cursiv::Extends<Stmt> {
  vector<Stmt *> stmts;
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

// No parens around the value, unlike `IfStmt`/`WhileStmt`.
struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr *value;
  Stmt *body;
};

struct CaseLabel : cursiv::Extends<Stmt> {
  Expr *value;
  Stmt *body;
};

struct LabeledStmt : cursiv::Extends<Stmt> {
  Ident *name;
  Stmt *body;
};

// The target is a full rvalue, not a name: B's `goto` takes any expression,
// a computed jump included.
struct GotoStmt : cursiv::Extends<Stmt> {
  Expr *target;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct ExternStmt : cursiv::Extends<Stmt> {
  vector<Ident *> names;
};

// One `auto` variable: a bare name, or one with a vector size after it.
struct AutoVar : cursiv::Extends<cursiv::AstNode> {
  boolean bracketed = false;
  Ident *name;
  Expr *size;
};

struct AutoStmt : cursiv::Extends<Stmt> {
  vector<AutoVar *> vars;
};

struct FunctionDef : cursiv::Extends<Item> {
  Ident *name;
  vector<Ident *> params;
  Stmt *body;
};

// A name definition outside a function: a scalar (no `size`, no `inits`), a
// sized vector (`size` only), or either with initial values (`inits`).
struct VarDef : cursiv::Extends<Item> {
  boolean bracketed = false;
  Ident *name;
  Expr *size;
  vector<Expr *> inits;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Item *> items;
};

} // namespace b::ast
