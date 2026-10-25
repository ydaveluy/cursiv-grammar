// The xpath language: its AST. XPath 3.1 is one expression grammar -- Expr
// is the common base, and the type system (SequenceType/ItemType) folds
// into two generic nodes since it is long-tail next to path expressions.
#pragma once

#include <cstdint>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace xpath::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// ---------------------------------------------------------------- literals

struct IntegerLit : cursiv::Extends<Expr> {
  string_view raw;
};

struct DecimalLit : cursiv::Extends<Expr> {
  string_view raw;
};

struct DoubleLit : cursiv::Extends<Expr> {
  string_view raw;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct VarRef : cursiv::Extends<Expr> {
  string_view name;
};

struct ContextItemExpr : cursiv::Extends<Expr> {};

struct SequenceExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct ParenExpr : cursiv::Extends<Expr> {
  SequenceExpr* body = nullptr;
};

// -------------------------------------------------------------------- types

// One generic node across the whole item-type grammar (node tests, map,
// array, function, named types): `name` carries the introducing keyword, the
// eqName of a named type, or is empty for a parenthesized item type.
struct ItemType : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> names;      // kindtest name/wildcard (0/1); PI target (0/1)
  string_view typeName;           // element/attribute's type annotation; map's key type
  bool nillable = false;          // element(name, type?) trailing '?'
  vector<struct SequenceType*> params;  // typedfunctiontest params; typed array/map value type
  SequenceType* returnType = nullptr;   // typedfunctiontest's return type
  ItemType* inner = nullptr;      // parenthesizeditemtype; documenttest's element constraint
};

struct SequenceType : cursiv::Extends<cursiv::AstNode> {
  bool emptySequence = false;
  ItemType* itemType = nullptr;
  string_view occurrence;  // "", "?", "*", "+"
};

struct SingleType : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  bool optional = false;
};

// ------------------------------------------------------------------ operators

enum class BinOp : std::uint8_t {
  or_,
  and_,
  concat,
  add,
  sub,
  mul,
  div_,
  idiv,
  mod,
  union_,
  intersect,
  except,
  eq,
  ne,
  lt,
  le,
  gt,
  ge,
  value_eq,
  value_ne,
  value_lt,
  value_le,
  value_gt,
  value_ge,
  node_is,
  node_precedes,
  node_follows,
  to,
  simple_map,
  path_child,
  path_descendant,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  BinOp op = BinOp::or_;
  Expr* left = nullptr;
  Expr* right = nullptr;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand = nullptr;
};

struct InstanceOfExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  SequenceType* type = nullptr;
};

struct TreatExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  SequenceType* type = nullptr;
};

struct CastableExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  SingleType* type = nullptr;
};

struct CastExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  SingleType* type = nullptr;
};

// ---------------------------------------------------------------------- arrow

struct ArrowStep : cursiv::Extends<cursiv::AstNode> {
  string_view name;        // arrowfunctionspecifier's bare-eqname form
  Expr* target = nullptr;  // varref or parenthesizedexpr form
  struct Arguments* args = nullptr;
};

struct ArrowExpr : cursiv::Extends<Expr> {
  Expr* base = nullptr;
  vector<ArrowStep*> steps;
};

// -------------------------------------------------------------------- paths

// nametest's raw spelling (eqname, '*', 'prefix:*', '*:local', 'Q{uri}*'), or
// a kindtest.
struct NodeTest : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  ItemType* kind = nullptr;
};

struct AxisStep : cursiv::Extends<Expr> {
  string_view axis;  // a forward/reverse axis word, "@", or empty (default child)
  NodeTest* test = nullptr;  // null only for the '..' abbreviation
  vector<Expr*> predicates;
};

// pathexpr's '/' and '//' forms; `rest` is null only for a bare '/'.
struct AbsolutePathExpr : cursiv::Extends<Expr> {
  bool doubleSlash = false;
  Expr* rest = nullptr;
};

// ----------------------------------------------------------------- postfix

struct Arguments : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> positional;  // may hold ArgPlaceholder*
};

struct ArgPlaceholder : cursiv::Extends<Expr> {};

struct CallExpr : cursiv::Extends<Expr> {
  string_view name;
  Arguments* args = nullptr;
};

struct Lookup : cursiv::Extends<Expr> {
  string_view key;          // NCName or IntegerLiteral spelling
  Expr* keyExpr = nullptr;  // parenthesizedexpr form
  bool wildcard = false;
};

using PostfixSuffix = std::variant<SequenceExpr*, Arguments*, Lookup*>;

struct PostfixExpr : cursiv::Extends<Expr> {
  Expr* base = nullptr;
  vector<PostfixSuffix> suffixes;
};

// ------------------------------------------------------------- constructors

struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* key = nullptr;
  Expr* value = nullptr;
};

struct MapConstructor : cursiv::Extends<Expr> {
  vector<MapEntry*> entries;
};

struct ArrayConstructor : cursiv::Extends<Expr> {
  bool curly = false;
  vector<Expr*> items;           // square form
  SequenceExpr* body = nullptr;  // curly form
};

struct NamedFunctionRef : cursiv::Extends<Expr> {
  string_view name;
  string_view arity;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  SequenceType* type = nullptr;
};

struct InlineFunctionExpr : cursiv::Extends<Expr> {
  vector<Param*> params;
  SequenceType* returnType = nullptr;
  SequenceExpr* body = nullptr;
};

// -------------------------------------------------------------------- flow

struct ForBinding : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* source = nullptr;
};

struct ForExpr : cursiv::Extends<Expr> {
  vector<ForBinding*> bindings;
  Expr* result = nullptr;
};

struct LetBinding : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value = nullptr;
};

struct LetExpr : cursiv::Extends<Expr> {
  vector<LetBinding*> bindings;
  Expr* result = nullptr;
};

struct QuantifierBinding : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* source = nullptr;
};

struct QuantifiedExpr : cursiv::Extends<Expr> {
  bool every = false;
  vector<QuantifierBinding*> bindings;
  Expr* predicate = nullptr;
};

struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond = nullptr;
  Expr* thenBranch = nullptr;
  Expr* elseBranch = nullptr;
};

// ---------------------------------------------------------------- document

// One or more `;`-separated expressions, the trailing `;` optional: the
// spec's own single bare expression is one entry with no separator, and the
// upstream corpus's multi-expression test files are the general case.
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> expressions;
};

}  // namespace xpath::ast
