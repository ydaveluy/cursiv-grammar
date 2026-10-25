// The stringtemplate language: its AST. A document is a run of elements
// (literal text and `<...>` constructs); an expression family covers what a
// `<...>` tag can hold.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace stringtemplate::ast {

// What `elements` holds: literal text, an expression tag, `if`, or a region.
struct Element : cursiv::Extends<cursiv::AstNode> {};

struct Text : cursiv::Extends<Element> {
  string_view value;
};

// The expression family living inside `<...>`. Empty by design: every
// operand member holds `Expr*`, so any alternative fits any operand slot.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
};

struct StringExpr : cursiv::Extends<Expr> {
  string_view value;
};

struct BoolExpr : cursiv::Extends<Expr> {
  boolean value;
};

struct NullExpr : cursiv::Extends<Expr> {};

struct ListExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

// An anonymous template: `{x, y | ...}`. `body` is the same element family
// the document itself is built from.
struct SubTemplateExpr : cursiv::Extends<Expr> {
  vector<string_view> params;
  vector<Element*> body;
};

// `expr.field`, one step of a member-access chain.
struct FieldAccess : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

// `expr.(expr)`, the computed-field-name step of the same chain.
struct IndexAccess : cursiv::Extends<Expr> {
  Expr* object;
  Expr* key;
};

// One call argument: positional (`value` alone), named (`name = value`), or
// the varargs marker `...` (`ellipsis`).
struct Arg : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
  boolean ellipsis;
};

struct ArgList : cursiv::Extends<cursiv::AstNode> {
  vector<Arg*> items;
};

struct CallExpr : cursiv::Extends<Expr> {
  string_view name;
  ArgList* args;
};

// `super.name(args)`.
struct SuperCallExpr : cursiv::Extends<Expr> {
  string_view name;
  ArgList* args;
};

// `@name()` and `@super.name()`: a reference to an enclosing region.
struct RegionRefExpr : cursiv::Extends<Expr> {
  string_view name;
  boolean isSuper;
};

// `(expr)(args)`: apply args to a template whose name is computed.
struct DynamicCallExpr : cursiv::Extends<Expr> {
  Expr* target;
  ArgList* args;
};

struct NotExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

enum class BoolOp : std::uint8_t { logical_and, logical_or };

struct BoolBinExpr : cursiv::Extends<Expr> {
  Expr* left;
  BoolOp op;
  Expr* right;
};

// One `:template` step of a map expression; several may chain.
struct MapStage : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> targets;
};

// `a, b : t1, t2 : t3` -- one or more zipped sources, then the chain of
// template stages applied to them.
struct MapExpr : cursiv::Extends<Expr> {
  vector<Expr*> sources;
  vector<MapStage*> stages;
};

struct Option : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct ExprTag : cursiv::Extends<Element> {
  Expr* value;
  vector<Option*> options;
};

struct IfBranch : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  vector<Element*> body;
};

struct IfStat : cursiv::Extends<Element> {
  vector<IfBranch*> branches;  // the `if`, then each `elseif`
  vector<Element*> elseBody;
  boolean hasElse;
};

struct Region : cursiv::Extends<Element> {
  string_view name;
  vector<Element*> elements;
};

struct Template : cursiv::Extends<cursiv::AstNode> {
  vector<Element*> elements;
};

}  // namespace stringtemplate::ast
