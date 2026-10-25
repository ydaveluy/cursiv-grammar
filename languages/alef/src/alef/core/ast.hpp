// The alef language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace alef::ast {

struct Decl : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct TypeSpec : cursiv::Extends<cursiv::AstNode> {};

// A pointer-depth marker: a run of `*`, one appended per star.
struct Pointer : cursiv::Extends<cursiv::AstNode> {};

struct ArgList;

// int/uint/sint/usint/byte/float/void/lint/ulint.
struct PrimType : cursiv::Extends<TypeSpec> {
  string_view keyword;
};

// A bare type name -- resolved against adt/aggr/union/enum/typedef names by a
// later pass, since the grammar has no symbol table.
struct NamedType : cursiv::Extends<TypeSpec> {
  string_view name;
};

struct TypeCast;

// `T[t1, t2]` -- a parametric adt instantiation.
struct ArrayOfType : cursiv::Extends<TypeSpec> {
  string_view name;
  vector<TypeCast*> args;
};

struct ChanType : cursiv::Extends<TypeSpec> {
  vector<TypeCast*> args;
  Expr* bufSize;
};

struct TName;

// A type as it appears in a cast, a parameter, or a tuple element: a base
// type with pointer depth, or a function-pointer signature, or a tuple type.
struct TypeCast : cursiv::Extends<cursiv::AstNode> {
  TypeSpec* type;
  vector<Pointer*> pointers;
  boolean isFuncType;
  vector<Pointer*> innerPointers;
  ArgList* params;
  TName* tuple;
};

// A declaration's type: an optional storage class, then a base type, a
// `tuple` type, or a parenthesised tuple of types -- at the head of a
// declaration, or in a tuple slot.
struct TName : cursiv::Extends<cursiv::AstNode> {
  string_view sclass;
  TypeSpec* type;
  vector<TypeCast*> tupleTypes;
  boolean tupleKeyword;
};

struct ArraySuffix : cursiv::Extends<cursiv::AstNode> {
  Expr* size;
};

// A name being declared: pointers, the name, array dims, and -- for a
// function-pointer variable -- a nested paren form with its own parameters.
struct Declarator : cursiv::Extends<cursiv::AstNode> {
  vector<Pointer*> pointers;
  string_view name;
  vector<ArraySuffix*> dims;
  boolean isFuncPtr;
  vector<Pointer*> innerPointers;
  ArgList* funcPtrParams;
};

struct InitList;

struct InitItem : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  Expr* index;
  Expr* indexedValue;
  string_view field;
  Expr* fieldValue;
  InitList* braced;
};

struct InitList : cursiv::Extends<cursiv::AstNode> {
  vector<InitItem*> items;
};

struct VarInit : cursiv::Extends<cursiv::AstNode> {
  Declarator* declarator;
  InitList* init;
};

// A function or function-pointer parameter, including a tuple-typed one and
// the two `*T`/`.T` variadic rest markers.
struct Arg : cursiv::Extends<cursiv::AstNode> {
  TypeSpec* type;
  TName* tupleType;
  vector<Pointer*> pointers;
  vector<ArraySuffix*> dims;
  boolean isFuncPtr;
  vector<Pointer*> innerPointers;
  ArgList* funcPtrParams;
  Declarator* declarator;
  boolean isVariadic;
  boolean isRestStar;
  boolean isRestDot;
};

struct ArgList : cursiv::Extends<cursiv::AstNode> {
  vector<Arg*> params;
};

struct AutoDecl : cursiv::Extends<cursiv::AstNode> {
  TypeSpec* type;
  TName* tupleType;
  vector<VarInit*> vars;
};

struct Block : cursiv::Extends<Stmt> {
  boolean bang;
  vector<AutoDecl*> autos;
  vector<Stmt*> stmts;
};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  boolean isDefault;
  vector<Stmt*> stmts;
};

struct CaseBody : cursiv::Extends<cursiv::AstNode> {
  boolean bang;
  vector<CaseClause*> cases;
};

struct TypeCaseClause : cursiv::Extends<cursiv::AstNode> {
  TypeCast* type;
  boolean isDefault;
  vector<Stmt*> stmts;
};

struct TypeCaseBody : cursiv::Extends<cursiv::AstNode> {
  boolean bang;
  vector<TypeCaseClause*> cases;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct CheckStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  string_view info;
};

struct AllocStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
};

struct UnallocStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
};

struct RescueStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  string_view name;
  Block* namedBlock;
};

struct RaiseStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct LabeledStmt;

// The target is a `LabeledStmt`, given whole-function label scope: a `goto`
// may cross into or out of a nested block.
struct GotoStmt : cursiv::Extends<Stmt> {
  reference<LabeledStmt> target;
};

struct ProcStmt : cursiv::Extends<Stmt> {
  vector<Expr*> args;
};

struct TaskStmt : cursiv::Extends<Stmt> {
  vector<Expr*> args;
};

struct BecomeStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct AltStmt : cursiv::Extends<Stmt> {
  CaseBody* body;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Expr* init;
  Expr* cond;
  Expr* step;
  Stmt* body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct DoStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct ParStmt : cursiv::Extends<Stmt> {
  Block* body;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  CaseBody* body;
};

struct TypeofStmt : cursiv::Extends<Stmt> {
  Expr* value;
  TypeCaseBody* body;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view level;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view level;
};

// `label: stmt` -- one label per statement, any statement.
struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view name;
  Stmt* body;
};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
  boolean dollar;
};

struct NilExpr : cursiv::Extends<Expr> {};

struct EllipsisExpr : cursiv::Extends<Expr> {};

// `.Module.field` -- an absolute, module-qualified reference.
struct QualifiedRef : cursiv::Extends<Expr> {
  string_view scope;
  string_view field;
};

// `(a, b, c)` and `{a, b, c}` -- a tuple literal or an lvalue tuple pattern;
// `braced` tells the two spellings apart.
struct TupleExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
  boolean braced;
};

struct SizeofTypeExpr : cursiv::Extends<Expr> {
  TypeCast* type;
};

struct SizeofExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean arrow;
  string_view field;
};

struct PostfixExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view op;
};

// `-x`, `*p`, `&x`, `<-c`, `!x`, `~m`, `++i`, `--i`, `?x`, `zerox x`.
struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// `(T)x`, `(alloc T)x` -- `isAlloc` tells the two apart.
struct CastExpr : cursiv::Extends<Expr> {
  TypeCast* type;
  Expr* operand;
  boolean isAlloc;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct VarDecl : cursiv::Extends<Decl> {
  TName* type;
  vector<VarInit*> vars;
};

struct FuncDecl : cursiv::Extends<Decl> {
  TName* returnType;
  NamedType* receiver;
  boolean receiverIndirect;
  Declarator* declarator;
  string_view methodName;
  ArgList* params;
  Block* body;
};

struct TypeDecl : cursiv::Extends<Decl> {
  TypeSpec* type;
};

struct TypedefDecl : cursiv::Extends<Decl> {
  TName* type;
  string_view bareKeyword;
  Declarator* declarator;
  ArgList* params;
  string_view opaqueName;
};

struct EnumMember : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct AggrType : cursiv::Extends<TypeSpec> {
  vector<string_view> tags;
  vector<Decl*> members;
};

struct UnionType : cursiv::Extends<TypeSpec> {
  vector<string_view> tags;
  vector<Decl*> members;
};

struct AdtType : cursiv::Extends<TypeSpec> {
  vector<string_view> tags;
  vector<string_view> typeParams;
  vector<Decl*> members;
};

struct EnumType : cursiv::Extends<TypeSpec> {
  vector<string_view> tags;
  vector<EnumMember*> members;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Decl*> decls;
};

}  // namespace alef::ast
