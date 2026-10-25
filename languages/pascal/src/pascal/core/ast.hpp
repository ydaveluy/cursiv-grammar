#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pascal::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct TypeNode : cursiv::Extends<cursiv::AstNode> {};
struct Decl : cursiv::Extends<cursiv::AstNode> {};

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
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

struct ActualArg : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  vector<Expr*> widths;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<ActualArg*> args;
};

struct NumberLit : cursiv::Extends<Expr> {
  string_view spelling;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view spelling;
};

struct NilLit : cursiv::Extends<Expr> {};

struct SetElement : cursiv::Extends<cursiv::AstNode> {
  Expr* low;
  Expr* high;
};

struct SetLit : cursiv::Extends<Expr> {
  vector<SetElement*> elements;
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

struct CompoundStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> statements;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  Expr* target;
  Expr* value;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct CaseArm : cursiv::Extends<cursiv::AstNode> {
  vector<SetElement*> labels;
  Stmt* body;
};

struct CaseStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<CaseArm*> arms;
  vector<Stmt*> elseBody;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct RepeatStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
  Expr* cond;
};

struct ForStmt : cursiv::Extends<Stmt> {
  string_view variable;
  Expr* from;
  Expr* iterable;
  boolean downto;
  Expr* to;
  Stmt* body;
};

struct WithStmt : cursiv::Extends<Stmt> {
  vector<Expr*> records;
  Stmt* body;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  string_view target;
};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* body;
};

struct NamedType : cursiv::Extends<TypeNode> {
  vector<string_view> names;
};

struct EnumType : cursiv::Extends<TypeNode> {
  vector<string_view> values;
};

struct SubrangeType : cursiv::Extends<TypeNode> {
  Expr* low;
  Expr* high;
};

struct StringType : cursiv::Extends<TypeNode> {
  Expr* size;
};

struct ArrayType : cursiv::Extends<TypeNode> {
  vector<TypeNode*> indices;
  TypeNode* component;
  boolean packed;
};

struct RecordField : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  TypeNode* type;
};

struct VariantPart;

struct VariantCase : cursiv::Extends<cursiv::AstNode> {
  vector<SetElement*> labels;
  vector<RecordField*> fields;
  VariantPart* variant;
};

struct VariantPart : cursiv::Extends<cursiv::AstNode> {
  string_view tagName;
  TypeNode* tagType;
  vector<VariantCase*> cases;
};

struct RecordType : cursiv::Extends<TypeNode> {
  vector<RecordField*> fields;
  VariantPart* variant;
  boolean packed;
};

struct SetType : cursiv::Extends<TypeNode> {
  TypeNode* base;
  boolean packed;
};

struct FileType : cursiv::Extends<TypeNode> {
  TypeNode* component;
  boolean packed;
};

struct PointerType : cursiv::Extends<TypeNode> {
  TypeNode* target;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  TypeNode* type;
  string_view kind;
  Expr* defaultValue;
};

struct ProcType : cursiv::Extends<TypeNode> {
  vector<Param*> params;
  TypeNode* result;
};

struct ConstDecl : cursiv::Extends<Decl> {
  string_view name;
  Expr* value;
  TypeNode* type;
};

struct TypeDecl : cursiv::Extends<Decl> {
  string_view name;
  TypeNode* type;
};

struct VarDecl : cursiv::Extends<Decl> {
  vector<string_view> names;
  TypeNode* type;
  Expr* value;
};

struct LabelDecl : cursiv::Extends<Decl> {
  vector<string_view> labels;
};

struct UsesItem : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> name;
  string_view sourcePath;
};

struct UsesDecl : cursiv::Extends<Decl> {
  vector<UsesItem*> units;
};

struct Block;

struct ProcDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<Param*> params;
  Block* body;
  vector<string_view> directives;
};

struct FuncDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<Param*> params;
  TypeNode* resultType;
  Block* body;
  vector<string_view> directives;
};

struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> decls;
  Stmt* body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> params;
  boolean library;
  Block* block;
};

struct Unit : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> name;
  vector<Decl*> interfaceDecls;
  vector<Decl*> implementationDecls;
  vector<Stmt*> initialization;
  vector<Stmt*> finalization;
};

}  // namespace pascal::ast
