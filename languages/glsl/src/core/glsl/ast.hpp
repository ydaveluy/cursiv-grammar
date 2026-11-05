

#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace glsl::ast {

struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Stmt : cursiv::Extends<Item> {};

struct Directive : cursiv::Extends<Item> {};

struct TypeArgument : cursiv::Extends<cursiv::AstNode> {};

struct Expr : cursiv::Extends<TypeArgument> {};

struct Ident : cursiv::Extends<Expr> {
  string name;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct TypeRef;

struct IndexExpr : cursiv::Extends<Expr> {
  Expr *array;
  Expr *index;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<Expr *> args;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr *object;
  string field;
};

struct PostfixOpExpr : cursiv::Extends<Expr> {
  Expr *operand;
  string op;
};

struct PrefixOpExpr : cursiv::Extends<Expr> {
  string op;
  Expr *operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  string op;
  Expr *right;
};

struct CondExpr : cursiv::Extends<Expr> {
  Expr *cond;
  Expr *thenValue;
  Expr *elseValue;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr *target;
  string op;
  Expr *value;
};

struct ArrayDim : cursiv::Extends<cursiv::AstNode> {
  Expr *size;
};

struct TypeQualifier;
struct Declarator;

struct StructMember : cursiv::Extends<Item> {
  TypeQualifier *qualifier;
  TypeRef *type;
  vector<Declarator *> declarators;
};

struct StructSpecifier : cursiv::Extends<cursiv::AstNode> {
  string name;
  vector<Item *> members;
};

struct TypeRef : cursiv::Extends<TypeArgument> {
  string name;
  StructSpecifier *structure;
  vector<TypeArgument *> parameters;
  vector<ArrayDim *> dims;
};

struct LayoutQualifierId : cursiv::Extends<cursiv::AstNode> {
  string name;
  Expr *value;
};

struct QualifierClause : cursiv::Extends<cursiv::AstNode> {
  string name;
  vector<LayoutQualifierId *> layout;
  vector<string> subroutineTypes;
};
struct TypeQualifier : cursiv::Extends<cursiv::AstNode> {
  vector<QualifierClause *> clauses;
};

struct Declarator : cursiv::Extends<cursiv::AstNode> {
  string name;
  vector<ArrayDim *> dims;
};

struct Initializer : cursiv::Extends<cursiv::AstNode> {};

struct ExprInit : cursiv::Extends<Initializer> {
  Expr *value;
};

struct ListInit : cursiv::Extends<Initializer> {
  vector<Initializer *> items;
};

struct InitDeclarator : cursiv::Extends<cursiv::AstNode> {
  string name;
  vector<ArrayDim *> dims;
  Initializer *init;
};

struct Declaration : cursiv::Extends<Item> {};

struct Param : cursiv::Extends<cursiv::AstNode> {
  TypeQualifier *qualifier;
  TypeRef *type;
  string name;
  vector<ArrayDim *> dims;
};

struct FunctionPrototype : cursiv::Extends<cursiv::AstNode> {
  TypeQualifier *qualifier;
  TypeRef *returnType;
  string name;
  vector<Param *> params;
};

struct FunctionDeclStmt : cursiv::Extends<Declaration> {
  FunctionPrototype *prototype;
};

struct VarDeclStmt : cursiv::Extends<Declaration> {
  TypeQualifier *qualifier;
  TypeRef *type;
  vector<InitDeclarator *> declarators;
};

struct PrecisionDeclStmt : cursiv::Extends<Declaration> {
  string precision;
  TypeRef *type;
};

struct InterfaceBlockDeclStmt : cursiv::Extends<Declaration> {
  TypeQualifier *qualifier;
  string blockName;
  vector<Item *> members;
  string instanceName;
  vector<ArrayDim *> instanceDims;
};

struct QualifierDeclStmt : cursiv::Extends<Declaration> {
  TypeQualifier *qualifier;
  vector<string> names;
};

struct NameStmt : cursiv::Extends<Item> {
  string name;
};

struct CompoundStmt;

struct FunctionDef : cursiv::Extends<Item> {
  FunctionPrototype *prototype;
  CompoundStmt *body;
};

struct CompoundStmt : cursiv::Extends<Stmt> {
  vector<Item *> items;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr *cond;
  Item *thenBranch;
  Item *elseBranch;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr *value;
  vector<Item *> items;
};

struct CaseLabelStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct DefaultLabelStmt : cursiv::Extends<Stmt> {};

struct Condition : cursiv::Extends<cursiv::AstNode> {
  Expr *expr;
  TypeQualifier *qualifier;
  TypeRef *type;
  string name;
  Initializer *init;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Condition *cond;
  Item *body;
};

struct DoWhileStmt : cursiv::Extends<Stmt> {
  Item *body;
  Expr *cond;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Item *init;
  Condition *cond;
  Expr *step;
  Item *body;
};

struct ContinueStmt : cursiv::Extends<Stmt> {};
struct BreakStmt : cursiv::Extends<Stmt> {};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr *value;
};

struct DiscardStmt : cursiv::Extends<Stmt> {};

struct TranslationUnit : cursiv::Extends<cursiv::AstNode> {
  vector<Item *> items;
};

struct RawDirective : cursiv::Extends<Directive> {
  string_view text;
};
struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};
struct Attribute : cursiv::Extends<cursiv::AstNode> {
  string name;
  vector<Expr *> args;
};
struct AttributedStmt : cursiv::Extends<Stmt> {
  vector<Attribute *> attributes;
  Item *body;
};
} // namespace glsl::ast
