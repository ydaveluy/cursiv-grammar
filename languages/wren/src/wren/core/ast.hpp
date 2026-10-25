// The wren language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace wren::ast {

struct Stat : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Block : cursiv::Extends<cursiv::AstNode> {
  boolean expressionBody;
  vector<string_view> params;
  vector<Stat*> statements;
};

// Atoms.
struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
};
struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};
struct NullExpr : cursiv::Extends<Expr> {};
struct BoolExpr : cursiv::Extends<Expr> {
  boolean value;
};
struct StringPart : cursiv::Extends<cursiv::AstNode> {};
struct StringText : cursiv::Extends<StringPart> {
  string_view value;
};
struct StringInterpolation : cursiv::Extends<StringPart> {
  Expr* expression;
};
struct StringExpr : cursiv::Extends<Expr> {
  boolean raw;
  vector<StringPart*> parts;
};
struct NumExpr : cursiv::Extends<Expr> {
  string_view text;
};
struct ListExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};
struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
};
struct MapExpr : cursiv::Extends<Expr> {
  vector<MapEntry*> entries;
};
struct BreakExpr : cursiv::Extends<Expr> {};
struct ContinueExpr : cursiv::Extends<Expr> {};

struct ImportVariable : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view alias;  // `as` target, or empty
};
// An import is both a file-level item and an expression: one class serves
// both spots.
struct ImportExpr : cursiv::Extends<Expr> {
  string_view module;
  vector<ImportVariable*> variables;
};

// The suffixed-expression chain: any primary, then any run of field/call/
// block-call/subscript suffixes, one flattened rule so a subscript can
// itself take a further suffix (`grid[0][1]`).
struct FieldExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};
struct BlockCallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  Block* block;
};
struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  vector<Expr*> indices;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};
// Every binary level of the operator ladder, `..`/`...` and `is` included:
// `op` holds the operator's text.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};
struct CondExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

// `var? target op value`; `value` is null for a bare `var name` declaration,
// and may itself be an AssignStat for a chained `a = b = c`.
struct AssignStat : cursiv::Extends<Expr> {
  boolean isVar;
  Expr* target;
  string_view op;
  Expr* value;
};

struct ExprStat : cursiv::Extends<Stat> {
  Expr* expr;
};
struct BlockStat : cursiv::Extends<Stat> {
  Block* body;
};
struct IfClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  Stat* body;
};
struct IfStat : cursiv::Extends<Stat> {
  vector<IfClause*> clauses;  // the `if`, then each `else if`
  Stat* elseBody;             // null when there is no `else`
};
struct WhileStat : cursiv::Extends<Stat> {
  Expr* cond;  // an expression, or an AssignStat
  Stat* body;
};
struct ForStat : cursiv::Extends<Stat> {
  string_view var;
  Expr* iterable;
  Stat* body;
};
struct ReturnStat : cursiv::Extends<Stat> {
  Expr* value;  // null for a bare `return`
};

// Class members: one base, one class per member shape (constructor,
// method, getter, setter, subscript getter/setter).
struct AttrValue : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;  // null when omitted
};
struct Attribute : cursiv::Extends<cursiv::AstNode> {
  boolean runtime;    // `#!` rather than `#`
  string_view group;  // the group name, or empty for a simple attribute
  vector<AttrValue*> values;
};
struct Member : cursiv::Extends<cursiv::AstNode> {
  boolean isForeign;
  boolean isStatic;
  vector<Attribute*> attributes;
};
struct MethodDef : cursiv::Extends<Member> {
  string_view name;
  vector<string_view> params;
  Block* body;  // null for a foreign method
};
struct ConstructorDef : cursiv::Extends<Member> {
  string_view name;
  vector<string_view> params;
  Block* body;
};
// A zero-argument getter, named or a unary operator overload
// (`toString { ... }`, `- { ... }`, `foreign static runtimeError`).
struct GetterDef : cursiv::Extends<Member> {
  string_view name;
  Block* body;  // null for a foreign getter
};
// A named property setter (`foo=(value) { ... }`) or a binary operator
// overload (`+(other) { ... }`), unified on one shape.
struct SetterDef : cursiv::Extends<Member> {
  string_view name;
  string_view param;
  Block* body;
};
struct SubscriptGetDef : cursiv::Extends<Member> {
  vector<string_view> params;
  Block* body;
};
struct SubscriptSetDef : cursiv::Extends<Member> {
  vector<string_view> params;
  string_view valueParam;
  Block* body;
};

struct ClassDef : cursiv::Extends<Stat> {
  boolean isForeign;
  string_view name;
  Expr* superclass;
  vector<Attribute*> attributes;
  vector<Member*> members;
};

struct Script : cursiv::Extends<cursiv::AstNode> {
  vector<Stat*> statements;
};

}  // namespace wren::ast
