// The cayenne language: its AST. One class per expression shape, sharing the
// abstract base `Expr`; a `data`/`sig`/`struct` body's items are plain
// AstNodes since they are not themselves expressions.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace cayenne::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

// '#', the type of types.
struct Hash : cursiv::Extends<Expr> {};

struct PiType : cursiv::Extends<Expr> {
  string_view param;
  Expr* paramType;
  Expr* body;
};

struct Lambda : cursiv::Extends<Expr> {
  string_view param;
  Expr* paramType;
  Expr* body;
};

// Juxtaposition: `func arg`, left-associative.
struct Application : cursiv::Extends<Expr> {
  Expr* func;
  Expr* arg;
};

struct FieldAccess : cursiv::Extends<Expr> {
  Expr* target;
  string_view field;
};

struct ConidAt : cursiv::Extends<Expr> {
  string_view name;
  Expr* type;
};

struct DataCtor : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> params;
};

struct DataExpr : cursiv::Extends<Expr> {
  vector<DataCtor*> ctors;
};

struct Sig : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* type;
  Expr* value;  // null when the signature carries no default
};

struct SigExpr : cursiv::Extends<Expr> {
  vector<Sig*> sigs;
};

struct Defn : cursiv::Extends<cursiv::AstNode> {
  boolean isPublic;
  boolean isAbstract;  // meaningful only when isPublic
  string_view name;
  Expr* type;
  Expr* value;
};

struct StructExpr : cursiv::Extends<Expr> {
  vector<Defn*> defns;
};

struct Arm : cursiv::Extends<cursiv::AstNode> {};

struct CtorArm : cursiv::Extends<Arm> {
  string_view ctor;
  vector<string_view> params;
  Expr* body;
};

struct VarArm : cursiv::Extends<Arm> {
  string_view name;
  Expr* body;
};

struct CaseExpr : cursiv::Extends<Expr> {
  string_view scrutinee;
  vector<Arm*> arms;
  Expr* type;
};

}  // namespace cayenne::ast
