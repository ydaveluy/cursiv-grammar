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
  vector<string_view> additionalImplementations;
  vector<Item*> decls;
};

// `names : type [= init] ;`. Module initializers receive a runtime error;
// ADT data/function members use AdtField without an initializer.
struct VarDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  Type* type;
  Expr* init;
};

struct ConDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  Expr* value;
};

struct ExceptionDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  vector<Type*> parameters;
};
struct ImportDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  string_view moduleName; // legacy simple name; use moduleExpression
  Expr* moduleExpression;
};

struct TypeDecl : cursiv::Extends<Item> {
  vector<string_view> names;
  Type* type;
};

struct IncludeDecl : cursiv::Extends<Item> {
  string_view path;
};

struct TopAssignment : cursiv::Extends<Item> {
  Expr* target;
  string_view op;
  Expr* value;
};
// Legacy flat top-level assignment classes; new parses use TopAssignment.
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

// ADT data/function fields retain the optional cyclic modifier.
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

struct GenericParameters : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
};
struct ConstraintArm : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  vector<AdtField*> fields;
};
struct RaiseSpec : cursiv::Extends<cursiv::AstNode> {
  boolean parenthesized;
  vector<Expr*> exceptions;
};
struct ModuleDecl : cursiv::Extends<Item> {
  string_view name;
  vector<string_view> additionalNames;
  vector<Item*> members;
};

struct AdtDecl : cursiv::Extends<Item> {
  string_view name;
  vector<string_view> additionalNames;
  GenericParameters* parameters;
  vector<ConstraintArm*> constraints;
  vector<Item*> members;
};

struct Signature;

struct FunctionDef : cursiv::Extends<Item> {
  vector<string_view> nameParts;
  vector<GenericParameters*> typeParameters;
  Signature* signature;
  RaiseSpec* raises;
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
  string_view selfType; // legacy simple name; see selfTypeExpression
  Type* selfTypeExpression;
  boolean isWildcard;
};

struct Signature : cursiv::Extends<cursiv::AstNode> {
  vector<FormalArg*> params;
  Type* result;
  vector<ConstraintArm*> constraints;
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

struct AppliedType : cursiv::Extends<Type> {
  Type* base;
  vector<Type*> arguments;
};
struct MemberType : cursiv::Extends<Type> {
  Type* base;
  string_view separator;
  string_view member;
};
struct CyclicType : cursiv::Extends<Type> {
  Type* base;
};
struct RefType : cursiv::Extends<Type> {
  Type* target;
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

struct FixedType : cursiv::Extends<Type> {
  vector<Expr*> parameters;
};
struct FnType : cursiv::Extends<Type> {
  GenericParameters* parameters;
  Signature* signature;
  RaiseSpec* raises;
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
struct TypeExpr : cursiv::Extends<Expr> {
  Type* type;
};
struct ExportSequence : cursiv::Extends<Expr> {
  vector<Expr*> values;
};
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
struct Qualifier;

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
  vector<Qualifier*> keys;
  Expr* a;
  Expr* b;
  boolean wildcardKey;
};

struct ListLit : cursiv::Extends<Expr> {
  vector<Expr*> elements;
};

struct ChanOfExpr : cursiv::Extends<Expr> {
  Expr* size;
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
  boolean equals;
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
  vector<string_view> additionalLabels;
  Expr* cond;
  Item* body;
};

struct DoWhileStmt : cursiv::Extends<Item> {
  string_view label;
  vector<string_view> additionalLabels;
  Item* body;
  Expr* cond;
};

struct ForStmt : cursiv::Extends<Item> {
  string_view label;
  vector<string_view> additionalLabels;
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
  vector<string_view> additionalLabels;
  Expr* value;
  vector<CaseArm*> arms;
};

struct AltStmt : cursiv::Extends<Item> {
  string_view label;
  vector<string_view> additionalLabels;
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
  vector<string_view> additionalLabels;
  string_view varName;
  Expr* value;
  vector<PickArm*> arms;
};

struct TryStmt : cursiv::Extends<Item> {
  Block* body;
  string_view variable;
  vector<CaseArm*> handlers;
};
struct RaiseStmt : cursiv::Extends<Item> {
  Expr* value;
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
  Expr* expression;
  Expr* callee; // legacy split representation; see expression
  vector<Expr*> args;
};

struct ExitStmt : cursiv::Extends<Item> {};

}  // namespace limbo::ast
