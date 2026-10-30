// The angelscript language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace angelscript::ast {

// Base of every script-level item, class/interface member, and statement:
// a block mixes declarations (Var) and statements freely, so they share one
// list element type.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// -- Types ----------------------------------------------------------------

struct TypeRef;

struct GenericArgs : cursiv::Extends<cursiv::AstNode> {
  vector<TypeRef*> args;
};

// One `name<generics>::` segment of a qualified name; every segment may
// carry generics here, not only the one immediately before a datatype.
struct ScopeSegment : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<GenericArgs*> generics;
};

struct TypeSuffix : cursiv::Extends<cursiv::AstNode> {};

struct ArraySuffix : cursiv::Extends<TypeSuffix> {};

struct HandleSuffix : cursiv::Extends<TypeSuffix> {
  boolean isConst;
};

struct TypeRef : cursiv::Extends<cursiv::AstNode> {
  boolean isConst;
  boolean rooted;  // a leading `::`
  vector<ScopeSegment*> scope;
  string_view name;  // datatype: an identifier, a primitive, `?` or `auto`
  vector<GenericArgs*> generics;
  vector<TypeSuffix*> suffixes;
};

// `('&' ('in'|'out'|'inout')?)?` of a parameter or lambda parameter.
struct TypeMod : cursiv::Extends<cursiv::AstNode> {
  boolean byRef;
  string_view dir;
};

// -- Expressions ------------------------------------------------------------

struct Arg : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // a named argument, `name: value`
  Expr* value;
};

struct ArgList : cursiv::Extends<cursiv::AstNode> {
  vector<Arg*> items;
};

struct VoidExpr : cursiv::Extends<Expr> {};

// NUMBER, STRING, BITS or a `true`/`false`/`null` keyword, kept as one node
// since nothing here inspects which kind matched.
struct LiteralExpr : cursiv::Extends<Expr> {
  string_view text;
};

struct ConstructExpr : cursiv::Extends<Expr> {
  TypeRef* type;
  ArgList* args;
};

struct CallExpr : cursiv::Extends<Expr> {
  boolean rooted;
  vector<ScopeSegment*> scope;
  string_view name;
  ArgList* args;
};

struct VarAccessExpr : cursiv::Extends<Expr> {
  boolean rooted;
  vector<ScopeSegment*> scope;
  string_view name;
};

struct CastExpr : cursiv::Extends<Expr> {
  TypeRef* type;
  Expr* operand;
};

struct LambdaParam : cursiv::Extends<cursiv::AstNode> {
  TypeRef* type;  // null when the parameter names only an identifier
  TypeMod* mod;
  string_view name;
};

struct BlockStmt;

struct LambdaExpr : cursiv::Extends<Expr> {
  vector<LambdaParam*> params;
  BlockStmt* body;
};

// A slot of an initializer list: an assignment expression, a nested list, or
// (dropped here) an empty slot between commas.
struct InitList;

struct InitListItem : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  InitList* nested;
};

struct InitList : cursiv::Extends<cursiv::AstNode> {
  vector<InitListItem*> items;
};

struct InitListExpr : cursiv::Extends<Expr> {
  TypeRef* type;  // the optional `type_ '='` prefix
  InitList* list;
};

// `object.name`
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
};

// `object.name(args)`, `object.ns::name(args)`
struct MemberCallExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean rooted;
  vector<ScopeSegment*> scope;
  string_view name;
  ArgList* args;
};

struct IndexArg : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // a named index argument, `name: value`
  Expr* value;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  vector<IndexArg*> items;
};

// A bare argument list applied as a postfix: calling the value the operand
// produced.
struct CallPostfixExpr : cursiv::Extends<Expr> {
  Expr* callee;
  ArgList* args;
};

struct PostIncDecExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view op;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

// Every level of the operator ladder, all left-associative: `op` holds the
// matched spelling, including a composed shift (`<<`, `>>`, `>>>`).
struct BinExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct CondExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

// -- Statements ---------------------------------------------------------

struct ExprStmt : cursiv::Extends<Item> {
  Expr* value;  // null for a bare `;`
};

struct BlockStmt : cursiv::Extends<Item> {
  vector<Item*> items;
};

struct IfStmt : cursiv::Extends<Item> {
  Expr* cond;
  Item* thenBranch;
  Item* elseBranch;
};

struct ForStmt : cursiv::Extends<Item> {
  Item* init;  // a VarDecl or an ExprStmt
  ExprStmt* cond;
  vector<Expr*> steps;
  Item* body;
};

struct WhileStmt : cursiv::Extends<Item> {
  Expr* cond;
  Item* body;
};

struct DoWhileStmt : cursiv::Extends<Item> {
  Item* body;
  Expr* cond;
};

struct ReturnStmt : cursiv::Extends<Item> {
  Expr* value;
};

struct BreakStmt : cursiv::Extends<Item> {};

struct ContinueStmt : cursiv::Extends<Item> {};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  Expr* value;  // null for `default`
  boolean isDefault;
  vector<Item*> body;
};

struct SwitchStmt : cursiv::Extends<Item> {
  Expr* value;
  vector<CaseClause*> cases;
};

struct TryStmt : cursiv::Extends<Item> {
  BlockStmt* tryBlock;
  BlockStmt* catchBlock;
};

// -- Declarations ---------------------------------------------------------

struct Param : cursiv::Extends<cursiv::AstNode> {
  TypeRef* type;
  TypeMod* mod;
  string_view name;
  Expr* defaultValue;
};

struct ParamList : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> params;
};

struct VarInit : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  InitList* initList;   // `= initlist`
  Expr* initExpr;       // `= expr`
  ArgList* ctorArgs;     // direct-init `arglist` form
};

struct VarDecl : cursiv::Extends<Item> {
  string_view visibility;  // "private", "protected" or empty
  TypeRef* type;
  vector<VarInit*> declarators;
};

struct ImportDecl : cursiv::Extends<Item> {
  TypeRef* returnType;
  boolean returnByRef;
  string_view name;
  ParamList* params;
  vector<string_view> attr;  // every function attribute, in source order
  string_view source;  // the `from STRING` target
};

struct EnumValue : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct EnumDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  string_view name;
  boolean forwardDecl;
  vector<EnumValue*> values;
};

struct TypedefDecl : cursiv::Extends<Item> {
  string_view primtype;
  string_view name;
};

struct FuncdefDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  TypeRef* returnType;
  boolean returnByRef;
  string_view name;
  ParamList* params;
};

struct AccessorDecl : cursiv::Extends<cursiv::AstNode> {
  boolean isSetter;  // false: a getter
  boolean isConst;
  vector<string_view> attr;  // every function attribute, in source order
  boolean declOnly;
  BlockStmt* body;
};

struct VirtPropDecl : cursiv::Extends<Item> {
  string_view visibility;
  TypeRef* type;
  boolean byRef;
  string_view name;
  vector<AccessorDecl*> accessors;
};

struct FuncDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  string_view visibility;
  TypeRef* returnType;   // null for a constructor
  boolean returnByRef;
  boolean isDestructor;
  string_view name;
  ParamList* params;
  boolean isConst;
  vector<string_view> attr;  // every function attribute, in source order
  boolean declOnly;
  BlockStmt* body;
};

struct IntfMthdDecl : cursiv::Extends<Item> {
  TypeRef* returnType;
  boolean returnByRef;
  string_view name;
  ParamList* params;
  boolean isConst;
};

struct ClassDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  string_view name;
  boolean forwardDecl;
  vector<string_view> bases;
  vector<Item*> members;  // VirtPropDecl, FuncDecl, VarDecl, FuncdefDecl
};

struct MixinDecl : cursiv::Extends<Item> {
  ClassDecl* class_;
};

struct InterfaceDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  string_view name;
  boolean forwardDecl;
  vector<string_view> bases;
  vector<Item*> members;  // VirtPropDecl, IntfMthdDecl
};

struct NamespaceDecl : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> items;
};

struct Script : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace angelscript::ast
