// The oberon language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace oberon::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct TypeNode : cursiv::Extends<cursiv::AstNode> {};
struct Decl : cursiv::Extends<cursiv::AstNode> {};

// -- expressions ----

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;  // a qualident: "Ident" or "Module.Ident"
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  vector<Expr*> indices;
};

struct FieldExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

struct DerefExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct ApplyExpr : cursiv::Extends<Expr> {
  Expr* operand;
  vector<Expr*> args;
};

struct TypeGuardExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view type;  // qualident
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct IntLit : cursiv::Extends<Expr> {
  int64 value;
};

struct RealLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NilLit : cursiv::Extends<Expr> {};

struct SetElement : cursiv::Extends<cursiv::AstNode> {
  Expr* low;
  Expr* high;  // null: a single value, not a range
};

struct SetLit : cursiv::Extends<Expr> {
  vector<SetElement*> elements;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;  // "+", "-", "~"
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// -- statements ----

struct AssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* value;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

// A statement slot that reads nothing: `a;` ends on one.
struct EmptyStmt : cursiv::Extends<Stmt> {};

// One `ELSIF cond THEN/DO body` clause, shared by IfStmt and WhileStmt.
struct ElsifClause : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
  vector<Stmt*> body;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> thenBranch;
  vector<ElsifClause*> elsifs;
  vector<Stmt*> elseBranch;
};

struct LabelRange : cursiv::Extends<cursiv::AstNode> {
  Expr* low;
  Expr* high;  // null: a single label, not a range
};

struct CaseArm : cursiv::Extends<cursiv::AstNode> {
  vector<LabelRange*> labels;
  vector<Stmt*> body;
};

struct CaseStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<CaseArm*> arms;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> body;
  vector<ElsifClause*> elsifs;
};

struct RepeatStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
  Expr* cond;
};

struct ForStmt : cursiv::Extends<Stmt> {
  string_view variable;
  Expr* from;
  Expr* to;
  Expr* by;  // null: no BY clause
  vector<Stmt*> body;
};

// -- types ----

struct NamedType : cursiv::Extends<TypeNode> {
  string_view name;  // qualident
};

struct ArrayType : cursiv::Extends<TypeNode> {
  vector<Expr*> lengths;
  TypeNode* component;
};

struct RecordField : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;  // identdef, star included in the text
  TypeNode* type;
};

struct RecordType : cursiv::Extends<TypeNode> {
  string_view base;  // qualident; empty: no extension
  vector<RecordField*> fields;
};

struct PointerType : cursiv::Extends<TypeNode> {
  TypeNode* target;
};

struct FormalParam : cursiv::Extends<cursiv::AstNode> {
  boolean byRef;  // VAR
  vector<string_view> names;
  string_view type;  // formalType: (ARRAY OF)* qualident, as text
};

struct FormalParams : cursiv::Extends<cursiv::AstNode> {
  vector<FormalParam*> params;
  string_view resultType;  // qualident; empty: a procedure, not a function
};

struct ProcedureType : cursiv::Extends<TypeNode> {
  FormalParams* params;  // null: no formal parameter list
};

// -- declarations ----

struct ConstDecl : cursiv::Extends<Decl> {
  string_view name;  // identdef
  Expr* value;
};

struct TypeDecl : cursiv::Extends<Decl> {
  string_view name;  // identdef
  TypeNode* type;
};

struct VarDecl : cursiv::Extends<Decl> {
  vector<string_view> names;  // identdef
  TypeNode* type;
};

struct ProcDecl : cursiv::Extends<Decl> {
  string_view name;  // identdef
  FormalParams* params;  // null: no formal parameter list
  vector<Decl*> decls;
  vector<Stmt*> body;
  Expr* returnExpr;  // null: no RETURN
  string_view endName;
};

struct Import : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view actual;  // empty unless "name := actual" renamed the import
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Import*> imports;
  vector<Decl*> decls;
  vector<Stmt*> body;
  string_view endName;
};

}  // namespace oberon::ast
