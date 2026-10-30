// The sysmlv2 language: its AST. SysML v2's textual notation names ~90
// definition/usage/relationship keywords over one shared shape (prefixes,
// an optional identification, specializations, a value, a body) -- Element
// is that shape, generic across the long tail; Expr and its few subclasses
// carry the expression language.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sysmlv2::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// A literal token spelling (real data rules join significant tokens): boolean,
// string, integer, real, `*`
// (infinity), `null`, `()`, or a block comment used as an expression
// placeholder.
struct Literal : cursiv::Extends<Expr> {
  string_view text;
};

struct NamedArgument : cursiv::Extends<Expr> {
  string_view name;
  Expr* value;
};

struct NameRef : cursiv::Extends<Expr> {
  string_view name;
};

struct Unary : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand = nullptr;
};

struct Binary : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  string_view op;
  Expr* right = nullptr;
};

struct Ternary : cursiv::Extends<Expr> {
  Expr* cond = nullptr;
  Expr* then_ = nullptr;
  Expr* else_ = nullptr;
};

struct Element;

// A sequence: a parenthesised expression list, `x[...]`, `x#(...)`.
struct Seq : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

// A `{ ... }` used as an expression (a function/calc/case body read as a
// value): items are member declarations, result is the trailing expression.
struct Body : cursiv::Extends<Expr> {
  vector<Element*> items;
  Expr* result = nullptr;
};

// The left-recursive postfix chain of a primary expression: member access
// (`.`, `.?`, `->`), indexing (`[...]`, `#(...)`), invocation (`(...)`) and
// `new` construction all share base/op/name/args/body -- which fields a
// given `op` fills is the only thing that varies.
struct Postfix : cursiv::Extends<Expr> {
  Expr* base = nullptr;
  string_view op;
  string_view name;
  vector<Expr*> args;
  Expr* body = nullptr;
  string_view functionReference;
};

// A single specialization/subsetting/typing/redefinition/conjugation edge:
// `kind` is the relation as spelled (":>", "subsets", "~", ...), `target`
// the qualified name (or dotted feature chain) it names.
struct Spec : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  string_view target;
  boolean isConjugated;
};

struct Endpoint : cursiv::Extends<cursiv::AstNode> {
  string_view multiplicity;
  string_view declaredName;
  boolean isEnd;
  string_view name;
  vector<Spec*> specs;
};
struct ActionClause : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  Endpoint* endpoint;
  Expr* expression;
};

// Every SysML definition, usage, relationship-with-a-body, action node,
// state action, transition, comment, doc, textual representation, import
// and alias: one shape, `kind` naming which.
struct Element : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> flags;         // visibility, abstract, direction, ref, var/const...
  vector<string_view> metaPrefixes;  // `#Type` annotations before the element
  string_view kind;                  // the leading keyword(s), or empty
  boolean isDef = false;              // a trailing `def` after the keyword
  string_view shortName;              // `<short>` in `<short> name`
  string_view name;                   // the identification's name
  vector<Spec*> specs;                 // typing/subsetting/redefinition/...
  string_view multiplicity;            // `[...]`, raw
  boolean isInitial;                  // := rather than =
  boolean isDefault;                  // default value
  boolean isParallel;                 // parallel state body
  Expr* value = nullptr;               // feature value / condition / result
  vector<Endpoint*> connectorEnds;
  vector<ActionClause*> clauses;
  vector<string_view> ends;            // connector/flow/dependency endpoints
  string_view lang;                    // decoded doc/rep locale or language
  string_view text;                    // processed comment/doc/rep body
  vector<Element*> members;            // body items
  Expr* result = nullptr;              // a body's trailing bare expression
  Expr* until = nullptr;              // trailing loop condition
  Element* alt = nullptr;              // an `if`'s `else` branch
};

struct RootNamespace : cursiv::Extends<cursiv::AstNode> {
  vector<Element*> members;
};

}  // namespace sysmlv2::ast
