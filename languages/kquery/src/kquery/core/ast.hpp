// The kquery language: its AST. A document is array declarations and query
// commands; every expression form is KLEE's own S-expression shape, so most
// operator families share one of two node classes and differ only by `op`.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace kquery::ast {

struct TranslationUnit : cursiv::Extends<cursiv::AstNode> {};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// Every numeric literal: a plain run of digits, a `0b`/`0o`/`0x` run, a
// signed one, or `true`/`false` -- told apart by their spelling, not by a
// field, since nothing downstream needs to compute with the value.
struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct VarRef : cursiv::Extends<Expr> {
  string_view name;
};

// `name:expression`, KLEE's let-binding: the name labels the value for reuse
// later in the same document, read back as a plain `VarRef`.
struct NamedExpr : cursiv::Extends<Expr> {
  string_view name;
  Expr* value;
};

// The arithmetic, bitwise and comparison families, and `Concat`: one shape,
// `op` holding the keyword as written (`Add`, `Eq`, `And`...). `width` is
// empty where the form omits it (comparisons, `Concat`).
struct BinaryExpr : cursiv::Extends<Expr> {
  string_view op;
  string_view width;
  Expr* left;
  Expr* right;
};

// The native reader also accepts Concat with one or more than two operands.
// Binary Concat keeps BinaryExpr for existing clients.
struct ConcatExpr : cursiv::Extends<Expr> {
  string_view width;
  vector<Expr*> operands;
};

// `Not w? e`, `Neg [w] e`, `ZExt w e`, `SExt w e`: one operand, `op` the
// keyword. `width` is empty only for `Neg`, whose form allows it.
struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  string_view width;
  Expr* operand;
};

// `(w e)`: a width paired with a number, KLEE's size query.
struct SizeExpr : cursiv::Extends<Expr> {
  string_view width;
  NumberLit* value;
};

// `(Extract w offset e)`.
struct ExtractExpr : cursiv::Extends<Expr> {
  string_view width;
  NumberLit* offset;
  Expr* value;
};

// `(Read|ReadLSB|ReadMSB w index array?)`. `array` is the array version being
// read, required for a valid native read, nullable only during recovery.
struct ReadExpr : cursiv::Extends<Expr> {
  string_view op;
  string_view width;
  Expr* index;
  Expr* array;
};

// `(Select w cond whenTrue whenFalse)`.
struct SelectExpr : cursiv::Extends<Expr> {
  string_view width;
  Expr* cond;
  Expr* whenTrue;
  Expr* whenFalse;
};

struct UpdatePair : cursiv::Extends<cursiv::AstNode> {
  Expr* index;
  Expr* value;
};

// `[idx=val, ...] @ base`: one array version built from another by a run of
// writes. `updates` may be empty (`[] @ base`).
struct UpdateExpr : cursiv::Extends<Expr> {
  vector<UpdatePair*> updates;
  Expr* base;
};

struct ArrayDecl : cursiv::Extends<TranslationUnit> {
  string_view name;
  string_view size;    // numArrayElements, raw text
  string_view domain;  // the index width, e.g. "w32"
  string_view range;   // the element width, e.g. "w8"
  boolean symbolic;
  vector<NumberLit*> values;  // the bracketed initializer, empty when symbolic
};

// `(query [constraints] queryExpr [evalExprs [evalArrayNames]])`.
struct QueryCommand : cursiv::Extends<TranslationUnit> {
  vector<Expr*> constraints;
  Expr* query;
  vector<Expr*> evalExprs;
  vector<string_view> evalArrayNames;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<TranslationUnit*> units;
};

}  // namespace kquery::ast
