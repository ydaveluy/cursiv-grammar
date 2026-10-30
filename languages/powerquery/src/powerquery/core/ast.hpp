// The powerquery language: its AST. Expr and TypeNode are the two families a
// document builds; a section or member's attribute values reuse Expr's own
// record/list/scalar nodes, since a literal value is exactly their shape,
// just parse-time restricted.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace powerquery::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct TypeNode : cursiv::Extends<cursiv::AstNode> {};

struct LogicalLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NumberLit : cursiv::Extends<Expr> {
  float64 value;
};

struct TextLit : cursiv::Extends<Expr> {
  string_view value;
};

struct NullLit : cursiv::Extends<Expr> {};

// Native verbatim literal #!"...", specified even though the Microsoft
// syntax package currently does not recognize it.
struct VerbatimLit : cursiv::Extends<Expr> {
  string_view value;
};

// `x`, `@x`. No reference<>: a let-expression's names may be mutually
// recursive, which the default scope cannot resolve as a simple forward link,
// so a name stays text here.
struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
  boolean inclusive;  // `@` prefix
};

struct SectionAccessExpr : cursiv::Extends<Expr> {
  string_view section;
  string_view member;
};

// `...` standing alone as an expression.
struct NotImplementedExpr : cursiv::Extends<Expr> {};

// One element of a list literal: `to` is null for a plain value, set for a
// `from .. to` range item.
struct ListItem : cursiv::Extends<cursiv::AstNode> {
  Expr* from;
  Expr* to;
};

struct ListExpr : cursiv::Extends<Expr> {
  vector<ListItem*> items;
};

struct Field : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct RecordExpr : cursiv::Extends<Expr> {
  vector<Field*> fields;
};

// `target[Field]`, `target[Field]?`, or bare `[Field]` when target is null
// (the implicit row context of a table transformation).
struct FieldAccess : cursiv::Extends<Expr> {
  Expr* target;
  string_view field;
  boolean optional;
};

// `target[[A],[B]]`, `target[[A],[B]]?`, or bare `[[A],[B]]` (target null).
struct Projection : cursiv::Extends<Expr> {
  Expr* target;
  vector<string_view> fields;
  boolean optional;
};

struct ItemAccess : cursiv::Extends<Expr> {
  Expr* target;
  Expr* index;
  boolean optional;
};

struct Invoke : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean optional;
  string_view name;
  TypeNode* type;  // null when unannotated
};

struct FunctionExpr : cursiv::Extends<Expr> {
  vector<Param*> params;
  TypeNode* returnType;  // null when unannotated
  Expr* body;
};

struct EachExpr : cursiv::Extends<Expr> {
  Expr* body;
};

struct Variable : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct LetExpr : cursiv::Extends<Expr> {
  vector<Variable*> variables;
  Expr* body;
};

struct IfExpr : cursiv::Extends<Expr> {
  Expr* condition;
  Expr* thenExpr;
  Expr* elseExpr;
};

struct ErrorExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct TryExpr : cursiv::Extends<Expr> {
  Expr* protectedExpr;
  Expr* otherwiseExpr;  // null when there is no `otherwise` clause
  FunctionExpr* catchFunction;
};

// `type <a type>`, a type value used where an expression is expected.
struct TypeValueExpr : cursiv::Extends<Expr> {
  TypeNode* type;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;  // "+", "-", "not"
  Expr* operand;
};

// The operation a binary level applies: every infix level of the ladder,
// unary excluded, shares this one enum and one node class.
enum class Op : std::uint8_t {
  meta,
  multiply,
  divide,
  add,
  subtract,
  concat,
  less,
  greater,
  less_equal,
  greater_equal,
  equal,
  not_equal,
  logical_and,
  logical_or,
  coalesce,
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Op op;
  Expr* right;
};

struct IsExpr : cursiv::Extends<Expr> {
  Expr* operand;
  TypeNode* type;
};

struct AsExpr : cursiv::Extends<Expr> {
  Expr* operand;
  TypeNode* type;
};

// Primitive type names are contextual keywords in a type position.
struct PrimitiveTypeNode : cursiv::Extends<TypeNode> {
  boolean nullable;  // set only when preceded by `nullable`
  string_view name;
};

struct FieldSpec : cursiv::Extends<cursiv::AstNode> {
  boolean optional;
  string_view name;
  TypeNode* type;  // null when unannotated
};

struct RecordTypeNode : cursiv::Extends<TypeNode> {
  vector<FieldSpec*> fields;
  boolean open;  // trailing `...`
};

struct ListTypeNode : cursiv::Extends<TypeNode> {
  TypeNode* itemType;
};

struct ParamSpec : cursiv::Extends<cursiv::AstNode> {
  boolean optional;
  string_view name;
  TypeNode* type;
};

struct FunctionTypeNode : cursiv::Extends<TypeNode> {
  vector<ParamSpec*> params;
  TypeNode* returnType;
};

struct TableTypeNode : cursiv::Extends<TypeNode> {
  vector<FieldSpec*> rows;
};

struct NullableTypeNode : cursiv::Extends<TypeNode> {
  TypeNode* inner;
};

// A primary expression in a nested type position, evaluated to a type at
// run time; type values and arbitrary values are distinguished semantically.
struct ComputedTypeNode : cursiv::Extends<TypeNode> {
  Expr* expr;
};

struct SectionMember : cursiv::Extends<cursiv::AstNode> {
  Expr* attributes;  // RecordExpr*, or null
  boolean shared;
  string_view name;
  Expr* value;
};

struct Section : cursiv::Extends<cursiv::AstNode> {
  Expr* attributes;  // RecordExpr*, or null
  string_view name;
  vector<SectionMember*> members;
};

// The document root: a document is either a section or a bare expression,
// so exactly one of the two is set.
struct Doc : cursiv::Extends<cursiv::AstNode> {
  Expr* expr;
  Section* section;
};

}  // namespace powerquery::ast
