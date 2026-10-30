// The matlab language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace matlab::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// Each operator token hands its own constant over, so `*` and `.*` are told
// apart by the field, not by comparing spellings.
enum class Operator : std::uint8_t {
  multiply,
  divide,
  ldivide,
  power,
  ewise_multiply,
  ewise_ldivide,
  ewise_rdivide,
  ewise_power,
  add,
  subtract,
  less,
  greater,
  less_equal,
  greater_equal,
  equal,
  not_equal,
  logical_and,
  logical_or,
  short_and,
  short_or,
  range,
};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct NumberLit : cursiv::Extends<Expr> {
  float64 value;
  boolean imaginary;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

// `(expr)`: kept as its own node, like the grouping every level below it
// could otherwise have absorbed.
struct ParenExpr : cursiv::Extends<Expr> {
  Expr *inner;
};

// Matrix rows are separated by semicolons or line breaks; each row keeps
// its columns in order, including whitespace-separated entries.
struct ArrayRow : cursiv::Extends<cursiv::AstNode> {
  vector<Expr *> elements;
};
struct ArrayLit : cursiv::Extends<Expr> {
  vector<ArrayRow *> rows;
};

// One argument of an indexed/called name: `:` alone (the whole range along
// that dimension) or an expression.
struct IndexArg : cursiv::Extends<cursiv::AstNode> {
  boolean all; // the bare `:` form; `value` is null when set
  Expr *value;
};

// `name(args)`: MATLAB writes array indexing and a function call with the
// same syntax, and nothing at the grammar level tells them apart.
struct IndexExpr : cursiv::Extends<Expr> {
  string_view name;
  vector<IndexArg *> args;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op; // + - ~
  Expr *operand;
};

// `x'`, `x .'`: a postfix transpose operator.
struct PostfixOpExpr : cursiv::Extends<Expr> {
  Expr *operand;
  string_view op;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  Operator op;
  Expr *right;
};

struct GlobalStmt : cursiv::Extends<Statement> {
  vector<string_view> names;
};

struct ClearStmt : cursiv::Extends<Statement> {
  vector<string_view> names;
};

// `target = value`: the target is restricted to what a postfix_expression
// can spell -- a name, an index/call, a transpose, or a parenthesised form.
struct AssignStmt : cursiv::Extends<Statement> {
  Expr *target;
  Expr *value;
};

// A bare separator alone (`;`, `,`, a line break) is a statement with
// nothing in it; `value` stays null for that case.
struct ExprStmt : cursiv::Extends<Statement> {
  Expr *value;
};

struct ElseIfClause : cursiv::Extends<cursiv::AstNode> {
  Expr *cond;
  vector<Statement *> body;
};

struct IfStmt : cursiv::Extends<Statement> {
  Expr *cond;
  vector<Statement *> thenBody;
  vector<ElseIfClause *> elseIfs;
  vector<Statement *> elseBody;
};

struct WhileStmt : cursiv::Extends<Statement> {
  Expr *cond;
  vector<Statement *> body;
};

// `for i = range ... end`, parens around the header optional: both forms
// reach this one node.
struct ForStmt : cursiv::Extends<Statement> {
  string_view var;
  Expr *range;
  vector<Statement *> body;
};

struct BreakStmt : cursiv::Extends<Statement> {};
struct ReturnStmt : cursiv::Extends<Statement> {};

struct ContinueStmt : cursiv::Extends<Statement> {};
struct FunctionDef : cursiv::Extends<Statement> {
  string_view name;
  vector<string_view> outputs;
  vector<string_view> parameters;
  vector<Statement*> body;
};

struct File : cursiv::Extends<cursiv::AstNode> {
  vector<Statement *> statements;
};

} // namespace matlab::ast
