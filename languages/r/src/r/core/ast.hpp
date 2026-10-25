// The r language: its AST. R has no statement/expression split -- an `if`,
// a `for`, an assignment and a block are all expressions -- so one Expr
// family covers everything a program is built from.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace r::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
  bool quoted = false;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;  // between the quotes, escapes raw
  bool raw = false;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct HexLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct ComplexLit : cursiv::Extends<Expr> {
  string_view text;
};

struct NullLit : cursiv::Extends<Expr> {};
struct NaLit : cursiv::Extends<Expr> {
  string_view text;
};
struct Placeholder : cursiv::Extends<Expr> {};
struct InfLit : cursiv::Extends<Expr> {};
struct NanLit : cursiv::Extends<Expr> {};
struct TrueLit : cursiv::Extends<Expr> {};
struct FalseLit : cursiv::Extends<Expr> {};
struct NextStmt : cursiv::Extends<Expr> {};
struct BreakStmt : cursiv::Extends<Expr> {};

struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

struct Block : cursiv::Extends<Expr> {
  vector<Expr*> body;
};

// Covers both `if (c) t` and `if (c) t else e`: elseBranch is null for the
// first.
struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenBranch;
  Expr* elseBranch;
};

struct ForExpr : cursiv::Extends<Expr> {
  string_view var;
  Expr* seq;
  Expr* body;
};

struct WhileExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* body;
};

struct RepeatExpr : cursiv::Extends<Expr> {
  Expr* body;
};

struct HelpExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

// `-x`, `!x`, `~x` (Sign, Not, ModelFormulaePrefix). The operator's text, so
// a consumer does not have to know the set.
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// Every infix level except assignment: `::`/`:::`, `$`/`@`, `^`, `:`,
// `%...%`, `*`/`/`, `+`/`-`, comparisons, `&`/`&&`, `|`/`||`, `~`.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// `<-`, `<<-`, `->`, `->>`, `:=`, `=`.
struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

// A formal parameter: `x`, `x = default`, `...` or `.` (dots/dot set, name
// empty).
struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* def = nullptr;
  boolean dots = false;
  boolean dot = false;
};

struct FunctionDef : cursiv::Extends<Expr> {
  vector<Param*> params;
  Expr* body;
};

// A call or subscript argument: positional (`value` set, `name` empty),
// named (`name` set, `value` optionally set) or one of `...`/`.`; every
// field may be empty, matching R's elided argument (`m[, 1]`).
struct Arg : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value = nullptr;
  boolean dots = false;
  boolean dot = false;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Arg*> args;
};

// `x[...]` and `x[[...]]`: one class, `listAccess` tells them apart, as
// `MemberExpr::arrow` does for `.`/`->` in the c example.
struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  vector<Arg*> args;
  boolean listAccess = false;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> body;
};

}  // namespace r::ast
