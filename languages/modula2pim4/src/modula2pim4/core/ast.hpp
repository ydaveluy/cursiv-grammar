// The modula2pim4 language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace modula2pim4::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct TypeNode : cursiv::Extends<cursiv::AstNode> {};
struct Decl : cursiv::Extends<cursiv::AstNode> {};
struct Field : cursiv::Extends<cursiv::AstNode> {};
struct CompilationUnit : cursiv::Extends<cursiv::AstNode> {};

// -- expressions ----

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;  // a qualident: "Ident" or "A.B.C"
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

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct IntLit : cursiv::Extends<Expr> {
  boolean character; // octal C literal has native type CHAR
  int64 value;
};

struct RealLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct SetElement : cursiv::Extends<cursiv::AstNode> {
  Expr* low;
  Expr* high;  // null: a single value, not a range
};

struct SetLit : cursiv::Extends<Expr> {
  Expr* baseType;  // null: an untyped set; else the qualident it followed
  vector<SetElement*> elements;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;  // "+", "-", "~", "NOT"
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

struct CallStmt : cursiv::Extends<Stmt> {
  Expr* value;  // a bare designator or a call
};

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

// caseLabelList's one element: shared by a record variant and a case
// statement's arm.
struct CaseLabelRange : cursiv::Extends<cursiv::AstNode> {
  Expr* low;
  Expr* high;  // null: a single label, not a range
};

struct CCase : cursiv::Extends<cursiv::AstNode> {
  vector<CaseLabelRange*> labels;
  vector<Stmt*> body;
};

struct CaseStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<CCase*> arms;
  vector<Stmt*> elseBranch;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  vector<Stmt*> body;
};

struct RepeatStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
  Expr* cond;
};

struct LoopStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
};

struct ForStmt : cursiv::Extends<Stmt> {
  string_view variable;
  Expr* from;
  Expr* to;
  Expr* by;  // null: no BY clause
  vector<Stmt*> body;
};

struct WithStmt : cursiv::Extends<Stmt> {
  Expr* target;
  vector<Stmt*> body;
};

struct ExitStmt : cursiv::Extends<Stmt> {};

// A statement slot that reads nothing: `a;` ends on one.
struct EmptyStmt : cursiv::Extends<Stmt> {};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;  // null: bare RETURN
};

// -- types ----

struct NamedType : cursiv::Extends<TypeNode> {
  string_view name;  // qualident
};

struct EnumType : cursiv::Extends<TypeNode> {
  vector<string_view> values;
};

struct SubrangeType : cursiv::Extends<TypeNode> {
  string_view base; // optional PIM4 base identifier before [low..high]
  Expr* low;
  Expr* high;
};

struct ArrayType : cursiv::Extends<TypeNode> {
  vector<TypeNode*> indexTypes;  // each a simpleType
  TypeNode* component;
};

struct RecordField : cursiv::Extends<Field> {
  vector<string_view> names;
  TypeNode* type;
};

// A field slot that reads nothing: `a: T;` ends on one.
struct EmptyField : cursiv::Extends<Field> {};

struct Variant : cursiv::Extends<cursiv::AstNode> {
  vector<CaseLabelRange*> labels;
  vector<Field*> fields;
};

// A variant (tagged-union) record field: `CASE tag: TagType OF ... END`.
struct CaseField : cursiv::Extends<Field> {
  string_view tag;      // the discriminant's name
  string_view tagType;  // qualident after ':' or '.'; empty if untyped
  vector<Variant*> variants;
  vector<Field*> elseFields;  // ELSE fieldListSequence; empty if no ELSE
};

struct RecordType : cursiv::Extends<TypeNode> {
  vector<Field*> fields;
};

struct SetType : cursiv::Extends<TypeNode> {
  TypeNode* base;
};

struct PointerType : cursiv::Extends<TypeNode> {
  TypeNode* target;
};

struct ProcTypeParam : cursiv::Extends<cursiv::AstNode> {
  boolean byRef;   // VAR
  string_view type;  // formalType: (ARRAY OF)? qualident, as text
};

struct FormalTypeList : cursiv::Extends<cursiv::AstNode> {
  vector<ProcTypeParam*> params;
  string_view resultType;  // qualident; empty: a procedure, not a function
};

struct ProcedureType : cursiv::Extends<TypeNode> {
  FormalTypeList* signature;  // null: bare PROCEDURE, no signature given
};

// -- declarations ----

struct ConstDecl : cursiv::Extends<Decl> {
  string_view name;
  Expr* value;
};

struct TypeDecl : cursiv::Extends<Decl> {
  string_view name;
  TypeNode* type;  // null: an opaque type (definition-module only)
};

struct VarDecl : cursiv::Extends<Decl> {
  vector<string_view> names;
  TypeNode* type;
};

struct FormalParam : cursiv::Extends<cursiv::AstNode> {
  boolean byRef;  // VAR
  vector<string_view> names;
  string_view type;  // formalType: (ARRAY OF)? qualident, as text
};

struct FormalParams : cursiv::Extends<cursiv::AstNode> {
  vector<FormalParam*> params;
  string_view resultType;  // qualident; empty: a procedure, not a function
};

// procedureHeading: also a Decl on its own, inside a definition module.
struct ProcHeading : cursiv::Extends<Decl> {
  string_view name;
  FormalParams* params;  // null: no formal parameter list
};

struct Block;

struct ProcDecl : cursiv::Extends<Decl> {
  ProcHeading* heading;
  Block* block;
  string_view endName;
};

struct Import : cursiv::Extends<cursiv::AstNode> {
  string_view from;  // empty: no FROM clause
  vector<string_view> names;
};

struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> decls;  // CONST/TYPE/VAR items, ProcDecl, ModuleDecl -- in
                         // the order the source declared them
  vector<Stmt*> body;    // empty: no BEGIN (`BEGIN END` holds an EmptyStmt)
};

// A nested (local) module declaration, itself a Decl of an enclosing block.
struct ModuleDecl : cursiv::Extends<Decl> {
  string_view name;
  Expr* priority;  // null: no priority
  vector<Import*> imports;
  boolean exportQualified;
  vector<string_view> exports;
  Block* block;
  string_view endName;
};

struct ProgramModule : cursiv::Extends<CompilationUnit> {
  boolean isImplementation;
  string_view name;
  Expr* priority;  // null: no priority
  vector<Import*> imports;
  Block* block;
  string_view endName;
};

struct DefinitionModule : cursiv::Extends<CompilationUnit> {
  string_view name;
  vector<Import*> imports;
  boolean exportQualified;
  vector<string_view> exports;
  vector<Decl*> decls;  // ConstDecl/TypeDecl/VarDecl/ProcHeading
  string_view endName;
};

}  // namespace modula2pim4::ast
