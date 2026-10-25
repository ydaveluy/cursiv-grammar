// The bicep language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace bicep::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct TypeExpr : cursiv::Extends<cursiv::AstNode> {};

struct Decorator : cursiv::Extends<cursiv::AstNode> {
  Expr* expression;
};

// -- Expressions ----------------------------------------------------------

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

// The spelling, not a computed value: an integer literal has no exponent or
// sign in this grammar, but can run past double precision.
struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NullLit : cursiv::Extends<Expr> {};

// `quasis.size() == expressions.size() + 1`, both pieces and holes read
// left to right.
struct InterpString : cursiv::Extends<Expr> {
  vector<string_view> quasis;
  vector<Expr*> expressions;
};

struct MultilineString : cursiv::Extends<Expr> {
  string_view text;
};

struct ArrayExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct ObjectProperty : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // set when the key is a plain or keyword-shaped name
  Expr* nameExpr;     // set when the key is an interpolated string instead
  Expr* value;
};

struct ObjectExpr : cursiv::Extends<Expr> {
  vector<ObjectProperty*> properties;
};

// `if (cond) { ... }`: the guard on a resource/module value, or on a `for`
// item.
struct IfExpr : cursiv::Extends<cursiv::AstNode> {
  Expr* condition;
  ObjectExpr* body;
};

struct ForExpr : cursiv::Extends<Expr> {
  string_view item;
  string_view index;  // empty for the single-identifier loop-variable form
  Expr* collection;
  Expr* body;    // forBody's expression alternative
  IfExpr* guard;  // forBody's ifCondition alternative -- exclusive with body
};

struct FunctionCall : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
};

struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

// `(a, b) => a + b`, or the bare-identifier shorthand for one parameter.
// Reuses `argumentList`'s expression shape for its parameters, as the source
// grammar does, so a parameter is an `Expr*` even though only an identifier
// is ever meaningful there.
struct LambdaExpr : cursiv::Extends<Expr> {
  vector<Expr*> params;
  Expr* body;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* index;
};

struct PropertyExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view property;
};

struct MethodCallExpr : cursiv::Extends<Expr> {
  Expr* object;
  FunctionCall* call;
};

// `memberExpression ":" identifier` -- unused by any corpus example, kept as
// its own node since the source grammar admits it beside indexing and
// property access.
struct NamedAccessExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

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

// -- Types ------------------------------------------------------------------

struct AmbientType : cursiv::Extends<TypeExpr> {
  string_view name;  // string, int, bool, array, object
};

struct TypeRef : cursiv::Extends<TypeExpr> {
  string_view name;
};

// `unaryOperator? literalValue` used as a type: a literal type such as `-1`
// or `'GET'`.
struct LiteralType : cursiv::Extends<TypeExpr> {
  string_view op;  // empty when absent
  Expr* value;
};

struct StringType : cursiv::Extends<TypeExpr> {
  string_view text;
};

struct MultilineStringType : cursiv::Extends<TypeExpr> {
  string_view text;
};

struct ArrayType : cursiv::Extends<TypeExpr> {
  TypeExpr* element;
};

struct ParenType : cursiv::Extends<TypeExpr> {
  TypeExpr* inner;
};

struct UnionType : cursiv::Extends<TypeExpr> {
  vector<TypeExpr*> options;
};

struct ObjectTypeProperty : cursiv::Extends<cursiv::AstNode> {
  vector<Decorator*> decorators;
  string_view name;
  TypeExpr* type;
};

struct ObjectTypeAdditionalProperties : cursiv::Extends<cursiv::AstNode> {
  vector<Decorator*> decorators;
  TypeExpr* type;
};

struct ObjectType : cursiv::Extends<TypeExpr> {
  // Order preserved, as the source grammar interleaves the two member kinds
  // freely inside one pair of braces.
  vector<std::variant<ObjectTypeProperty*, ObjectTypeAdditionalProperties*>>
      members;
};

struct TupleItem : cursiv::Extends<cursiv::AstNode> {
  vector<Decorator*> decorators;
  TypeExpr* type;
};

struct TupleType : cursiv::Extends<TypeExpr> {
  vector<TupleItem*> items;
};

// -- Statements ---------------------------------------------------------

struct TargetScopeDecl : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ImportDecl : cursiv::Extends<Stmt> {
  vector<Decorator*> decorators;
  Expr* specification;
  ObjectExpr* withClause;  // null when absent
  string_view alias;        // empty when absent
};

struct MetadataDecl : cursiv::Extends<Stmt> {
  string_view name;
  Expr* value;
};

struct ParameterDecl : cursiv::Extends<Stmt> {
  vector<Decorator*> decorators;
  string_view name;
  TypeExpr* type;          // the typeExpression form
  Expr* resourceType;       // the "resource" interpString form -- exclusive
  Expr* defaultValue;        // absent as null
};

struct TypeDecl : cursiv::Extends<Stmt> {
  vector<Decorator*> decorators;
  string_view name;
  TypeExpr* type;
};

struct VariableDecl : cursiv::Extends<Stmt> {
  vector<Decorator*> decorators;
  string_view name;
  Expr* value;
};

struct ResourceDecl : cursiv::Extends<Stmt> {
  vector<Decorator*> decorators;
  string_view name;
  Expr* type;
  boolean existing;
  ObjectExpr* object;  // exactly one of these three is set
  IfExpr* ifValue;
  ForExpr* forValue;
};

struct ModuleDecl : cursiv::Extends<Stmt> {
  vector<Decorator*> decorators;
  string_view name;
  Expr* type;
  ObjectExpr* object;  // exactly one of these three is set
  IfExpr* ifValue;
  ForExpr* forValue;
};

struct OutputDecl : cursiv::Extends<Stmt> {
  vector<Decorator*> decorators;
  string_view name;
  string_view typeName;    // the plain-identifier type form
  Expr* resourceType;       // the "resource" interpString form -- exclusive
  Expr* value;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> body;
};

}  // namespace bicep::ast
