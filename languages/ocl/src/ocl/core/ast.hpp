// The OCL language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ocl::ast {

struct Type : cursiv::Extends<cursiv::AstNode> {};

// Sequence/Set/Bag/OrderedSet/Collection/Ref (one arg), Map/Function (two).
struct ParamType : cursiv::Extends<Type> {
  string_view keyword;
  vector<Type*> args;
};

struct NamedType : cursiv::Extends<Type> {
  string_view name;
};

struct IdentOptType;
struct TupleType : cursiv::Extends<Type> {
  vector<IdentOptType*> parts;
};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct NullLit : cursiv::Extends<Expr> {};
struct InvalidLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct IntLit : cursiv::Extends<Expr> {
  int64 value;
};

struct FloatLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct TuplePart : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type = nullptr;
  Expr* value;
};
struct TupleLiteral : cursiv::Extends<Expr> {
  vector<TuplePart*> parts;
};

// `Color::red`: an enum literal's qualifier and member lex as one token.
struct EnumLit : cursiv::Extends<Expr> {
  string_view text;
};

struct NameRef : cursiv::Extends<Expr> {
  boolean isSelf = false;
  string_view name;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// One shape for the four infix levels (logical, equality, additive,
// multiplicative): `op` carries the spelling, never a separate constant.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenExpr;
  Expr* elseExpr;
};

struct LetBinding : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type = nullptr;
  Expr* value;
};

struct LetExpr : cursiv::Extends<Expr> {
  vector<LetBinding*> bindings;
  Expr* body;
};

struct SetLiteral : cursiv::Extends<Expr> {
  string_view kind;  // Set, Bag, Sequence, OrderedSet, Map
  vector<Expr*> items;
};

// A `id (: type)?` binder, bare or bound: collect/select/.../iterate.
struct IdentOptType : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type = nullptr;
};

struct MemberAccess : cursiv::Extends<Expr> {
  Expr* object;
  boolean arrow = false;
  string_view name;
};

// `.name(args)` and `->name(args)` alike, lambda-taking operations
// included: `params` holds the optional binder(s) before `|`.
struct Call : cursiv::Extends<Expr> {
  Expr* object;
  boolean arrow;
  string_view name;
  vector<IdentOptType*> params;
  vector<Expr*> args;
};

struct IterateCall : cursiv::Extends<Expr> {
  Expr* object;
  string_view accumVar;  // legacy name: first iterator variable
  Type* iteratorType = nullptr;
  IdentOptType* accumParam;
  Expr* init;
  Expr* body;
};

// A bare name applied to arguments, e.g. `commonSuperType(p.type)`. Anchored
// to a name, not a general callee: chained onto any postfix suffix instead,
// `.name(args)` would derive two ways -- one Call, or MemberAccess then this
// -- and the parse forks at every step of a chain.
struct DirectCall : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* index;
};

struct Spec : cursiv::Extends<cursiv::AstNode> {};

struct Invariant : cursiv::Extends<Spec> {
  string_view contextName;
  string_view name;  // optional
  Expr* body;
};

struct DerivedAttribute : cursiv::Extends<Spec> {
  string_view qualifiedName;  // A::b, whole
  Type* type;
  Expr* init = nullptr;
  Expr* derive;
};

struct Model : cursiv::Extends<cursiv::AstNode> {
  vector<Spec*> specs;
};

}  // namespace ocl::ast
