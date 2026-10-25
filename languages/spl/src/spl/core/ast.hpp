// The SPL language: its AST. `Expr` is the arithmetic/value family (numbers,
// strings, dates, globs, paths, calls); `Operation` is a search-pipeline
// predicate (comparisons, LIKE/IN, BY/AS/OUTPUT, boolean combinators).
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace spl::ast {

enum class ArithOp : std::uint8_t { power, multiply, modulo, divide, add, subtract };
enum class CompareOp : std::uint8_t { eq, ne, gt, lt, ge, le };
enum class BoolOp : std::uint8_t { and_, or_ };

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct ArithExpr : cursiv::Extends<Expr> {
  Expr* left;
  ArithOp op;
  Expr* right;
};

struct FuncCall : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
};

// A glob value: `*`, `*.html`, `*(a+b)*`. `inner` is absent for a bare `*`.
struct Glob : cursiv::Extends<Expr> {
  boolean leadingStar;
  Expr* inner;
  boolean trailingStar;
};

// `/var/log`: a slash-separated path, one or more segments.
struct PathExpr : cursiv::Extends<Expr> {
  vector<string_view> segments;
};

struct DateLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

// A bare name used as a value: a field, a command word, a function word.
struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
};

struct NumberLit : cursiv::Extends<Expr> {
  boolean negative;
  double value;
};

struct Operation : cursiv::Extends<cursiv::AstNode> {};

struct BoolCombine : cursiv::Extends<Operation> {
  Operation* left;
  BoolOp op;
  Operation* right;
};

struct Negation : cursiv::Extends<Operation> {
  Operation* operand;
};

struct LikeTest : cursiv::Extends<Operation> {
  Expr* subject;
  Expr* pattern;
};

struct InTest : cursiv::Extends<Operation> {
  Expr* subject;
  vector<Expr*> options;
};

// `lookup ... OUTPUT a`, `... a b OUTPUT x, y`, `... a b c OUTPUTNEW x, y, z`:
// as many sources as targets, one to three of each.
struct OutputClause : cursiv::Extends<Operation> {
  vector<Expr*> sources;
  boolean isNew;
  vector<string_view> targets;
};

struct ByClause : cursiv::Extends<Operation> {
  vector<string_view> fields;
};

struct Alias : cursiv::Extends<Operation> {
  Expr* value;
  string_view name;
};

struct Comparison : cursiv::Extends<Operation> {
  string_view field;
  CompareOp op;
  Expr* value;
};

struct BareExpr : cursiv::Extends<Operation> {
  Expr* value;
};

struct InitCommand;
struct NextCommand;

// `[ initCommand (| nextCommand)* ]`: a search embedded as a value, e.g.
// `join user [ search index=users ]`.
struct Subquery : cursiv::Extends<cursiv::AstNode> {
  InitCommand* init;
  vector<NextCommand*> pipeline;
};

// The first stage of a query or subquery: its leading command name is one of
// the words that may open a search (`search`, `from`, `tstats`...), or
// absent for a bare search (`index=web ...`).
struct InitCommand : cursiv::Extends<cursiv::AstNode> {
  string_view commandName;
  vector<Operation*> operations;
  Subquery* subquery;
};

// A stage after a `|`: its command name is required, and is any command word.
struct NextCommand : cursiv::Extends<cursiv::AstNode> {
  string_view commandName;
  vector<Operation*> operations;
  Subquery* subquery;
};

struct Query : cursiv::Extends<cursiv::AstNode> {
  InitCommand* init;
  vector<NextCommand*> pipeline;
};

}  // namespace spl::ast
