// The Limbo language: its AST. `Item` is the family every declaration and
// statement extends -- a program body, a module body, an adt body and a
// block all hold a `vector<Item*>`, the grammar keeping the wrong shapes out
// of the wrong list, not this type.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace limbo::ast {

struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Type : cursiv::Extends<cursiv::AstNode> {};

struct Program : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Item*> decls;
};

// `names : type [= init] ;`. Reused for a module's data/function members and
// an adt's function-pointer members, which real Limbo gives no initializer,
// but lose nothing by accepting one here.
struct VarDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  Type* type;
  Expr* init;
};

struct ConDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  Expr* value;
};

struct ImportDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  string_view moduleName;
};

struct TypeDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  Type* type;
};

struct IncludeDecl : cursiv::Extends<Item> {
  string_view path;
};

struct ShortVarDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  Expr* value;
};

struct AssignDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  Expr* value;
};

struct TupleShortVarDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  Expr* value;
};

// An adt field declared `cyclicopt` requires the keyword, not an optional
// one, so it is kept apart from the plain `VarDecl` shape.
struct AdtField : cursiv::Extends<Item> {
  boolean cyclic;
  vector<string_view> names;
  Type* type;
};

struct PickTagArm : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> tags;
  vector<AdtField*> fields;
};

struct PickDecl : cursiv::Extends<Item> {
  vector<PickTagArm*> arms;
};

struct ModuleDecl : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> members;
};

struct AdtDecl : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> members;
};

struct Signature;

struct FunctionDef : cursiv::Extends<Item> {
  vector<string_view> nameParts;
  Signature* signature;
  vector<Item*> body;
};

// A `names : type` parameter (`nil` a legal name, meaning "unused"), a
// `self [refopt] T` receiver, or a bare `*` (varargs marker): one class,
// told apart by `isSelf`/`isWildcard`.
struct FormalArg : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  Type* type;
  boolean isSelf;
  boolean refopt;
  string_view selfType;
  boolean isWildcard;
};

struct Signature : cursiv::Extends<cursiv::AstNode> {
  vector<FormalArg*> params;
  Type* result;
};

struct PrimType : cursiv::Extends<Type> {
  string_view keyword;
};

// `id1` alone is a bare name; `id2` set means `id1 -> id2`, a
// module-qualified type.
struct NamedType : cursiv::Extends<Type> {
  string_view id1;
  string_view id2;
};

struct RefType : cursiv::Extends<Type> {
  NamedType* target;
};

struct TupleType : cursiv::Extends<Type> {
  vector<Type*> elements;
};

struct ArrayOfType : cursiv::Extends<Type> {
  Type* element;
};

struct ListOfType : cursiv::Extends<Type> {
  Type* element;
};

struct ChanOfType : cursiv::Extends<Type> {
  Type* element;
};

struct FnType : cursiv::Extends<Type> {
  Signature* signature;
};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct RealLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

struct NilExpr : cursiv::Extends<Expr> {};

struct ParenExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

struct ModAccessExpr : cursiv::Extends<Expr> {
  Expr* module_;
  Expr* member;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

// `a[i]`, `a[i:]` and `a[i:j]`: one class, `isSlice` set by the `:`.
struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
  Expr* high;
  boolean isSlice;
};

struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct CastExpr : cursiv::Extends<Expr> {
  Type* type;
  Expr* operand;
};

struct InitElem;

// `array [size] of T`, or `array [size?] of { a, b, ... }`: one class,
// `hasInit` set by the brace form.
struct ArrayExpr : cursiv::Extends<Expr> {
  Expr* size;
  Type* elemType;
  vector<InitElem*> elements;
  boolean hasInit;
};

// `a`, or `k => a`, or `* => a`: `b` null means `a` is the value alone;
// `b` set means `a` is the key and `b` the value.
struct InitElem : cursiv::Extends<cursiv::AstNode> {
  Expr* a;
  Expr* b;
  boolean wildcardKey;
};

struct ListLit : cursiv::Extends<Expr> {
  vector<Expr*> elements;
};

struct ChanOfExpr : cursiv::Extends<Expr> {
  Type* elemType;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

struct SendExpr : cursiv::Extends<Expr> {
  Expr* target;
  Expr* value;
};

struct DeclareExpr : cursiv::Extends<Expr> {
  Expr* target;
  Expr* value;
};

struct LoadExpr : cursiv::Extends<Expr> {
  string_view module_;
  Expr* path;
};

struct ExprStmt : cursiv::Extends<Item> {
  Expr* value;
};

struct Block : cursiv::Extends<Item> {
  vector<Item*> stmts;
};

struct IfStmt : cursiv::Extends<Item> {
  Expr* cond;
  Item* thenBranch;
  Item* elseBranch;
};

struct WhileStmt : cursiv::Extends<Item> {
  string_view label;
  Expr* cond;
  Item* body;
};

struct DoWhileStmt : cursiv::Extends<Item> {
  string_view label;
  Item* body;
  Expr* cond;
};

struct ForStmt : cursiv::Extends<Item> {
  string_view label;
  Expr* init;
  Expr* cond;
  Expr* post;
  Item* body;
};

// A case/alt label: `*` (wildcard), `a` (`b` null), or `a to b` (a range).
struct Qualifier : cursiv::Extends<cursiv::AstNode> {
  Expr* a;
  Expr* b;
  boolean wildcard;
};

struct CaseArm : cursiv::Extends<cursiv::AstNode> {
  vector<Qualifier*> labels;
  vector<Item*> body;
};

struct CaseStmt : cursiv::Extends<Item> {
  string_view label;
  Expr* value;
  vector<CaseArm*> arms;
};

struct AltStmt : cursiv::Extends<Item> {
  string_view label;
  vector<CaseArm*> arms;
};

struct PickQual : cursiv::Extends<cursiv::AstNode> {
  string_view tag;
  boolean wildcard;
};

struct PickArm : cursiv::Extends<cursiv::AstNode> {
  vector<PickQual*> tags;
  vector<Item*> body;
};

struct PickStmt : cursiv::Extends<Item> {
  string_view label;
  string_view varName;
  Expr* value;
  vector<PickArm*> arms;
};

struct BreakStmt : cursiv::Extends<Item> {
  string_view label;
};

struct ContinueStmt : cursiv::Extends<Item> {
  string_view label;
};

struct ReturnStmt : cursiv::Extends<Item> {
  Expr* value;
};

struct SpawnStmt : cursiv::Extends<Item> {
  Expr* callee;
  vector<Expr*> args;
};

struct ExitStmt : cursiv::Extends<Item> {};

}  // namespace limbo::ast
