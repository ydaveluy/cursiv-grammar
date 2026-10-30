// The apex language: its AST.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace apex::ast {

struct TypeRef;
struct TypeArg;
struct Expr;
struct Item;
struct Initializer;

// -- Annotations ----

struct AnnotationValue : cursiv::Extends<cursiv::AstNode> {};

struct AnnotationExprValue : cursiv::Extends<AnnotationValue> {
  Expr* value;
};

struct AnnotationArrayValue : cursiv::Extends<AnnotationValue> {
  vector<AnnotationValue*> items;
};

struct AnnotationArg : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // empty for a single positional value
  AnnotationValue* value;
};

struct Annotation : cursiv::Extends<AnnotationValue> {
  string_view name;
  vector<AnnotationArg*> args;
};

// -- Types (usage) ----

struct TypeRef : cursiv::Extends<cursiv::AstNode> {};

struct PrimitiveTypeRef : cursiv::Extends<TypeRef> {
  string_view keyword;
};

struct VoidTypeRef : cursiv::Extends<TypeRef> {};

// One dotted segment of a class/interface type, nesting outward, as in
// java's port: `Outer<T>.Inner<U>` is a ClassTypeRef "Inner" whose qualifier
// is "Outer".
struct ClassTypeRef : cursiv::Extends<TypeRef> {
  ClassTypeRef* qualifier;
  string_view name;
  vector<TypeArg*> typeArguments;
};

// One array dimension wrapping its element type; `Integer[][]` nests two.
struct ArrayTypeRef : cursiv::Extends<TypeRef> {
  TypeRef* element;
};

struct TypeArg : cursiv::Extends<cursiv::AstNode> {};

struct TypeArgType : cursiv::Extends<TypeArg> {
  TypeRef* type;
};

struct WildcardTypeArg : cursiv::Extends<TypeArg> {
  boolean superBound;  // true: `? super T`; false: `? extends T`
  TypeRef* bound;       // null: unbounded `?`
};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TypeRef*> bounds;
};

// -- Literals / expressions ----

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct ThisExpr : cursiv::Extends<Expr> {};
struct SuperExpr : cursiv::Extends<Expr> {};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};
struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};
struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};
struct BoolLit : cursiv::Extends<Expr> {
  string_view text;
};
struct NullLit : cursiv::Extends<Expr> {};

struct ClassLit : cursiv::Extends<Expr> {
  TypeRef* type;
};

// A SOQL/SOSL query, `[SELECT ...]` or `[FIND ...]`: one opaque leaf.
// Quote-delimited ] is retained; embedded Apex bind arrays are not parsed.
struct SoqlExpr : cursiv::Extends<Expr> {
  string_view text;
};

// `<T>this(...)`, `<T>super(...)`, `<T>name(...)`, the qualified
// `expr.<T>super.member(...)`, and `expr.<T>name(...)`.
struct ExplicitGenericInvokeExpr : cursiv::Extends<Expr> {
  Expr* outer;  // set for the qualified `expr.<T>...` form
  vector<TypeArg*> typeArguments;
  string_view target;  // "this", "super", or a method name
  string_view member;  // set for `<T>super.member(...)`
  vector<Expr*> args;
  boolean invoked;  // an argument list followed
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};

// `field` also covers `.this`, `.get`/`.set` (a call always follows through
// the ordinary postfix `(...)`, since a dedicated `.get(...)`/`.set(...)`
// alternative would shape identically) and the DML words used as a member
// name (`.insert`, `.upsert`, `.update`, `.delete`, `.undelete`), reserved
// words that are otherwise only usable here.
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean safe;  // `?.`
  string_view field;
};

// Bare `expr.new`: no creator follows.
struct QualifiedNewExpr : cursiv::Extends<Expr> {
  Expr* outer;
};

struct QualifiedSuperExpr : cursiv::Extends<Expr> {
  Expr* outer;
  string_view member;  // empty: `expr.super(...)`; set: `expr.super.member`
  boolean invoked;
  vector<Expr*> args;
};

struct NewObjectExpr : cursiv::Extends<Expr> {
  vector<TypeArg*> typeArguments;
  TypeRef* type;
  vector<Expr*> args;
  boolean anonymousBody;
  vector<Item*> classBody;
};

struct NewArrayExpr : cursiv::Extends<Expr> {
  TypeRef* elementType;
  vector<Expr*> dimExprs;  // sized dimensions; a trailing `[]` adds none
  Initializer* init;
};

// `new Map<K, V>{ k1 => v1, k2 => v2 }` (`values` parallel to `keys`) or
// `new Set<T>{ v1, v2 }` (`keys` populated, `values` empty). An empty
// literal (`T{}`) is a valid, indistinguishable spelling of either, so one
// node lets it go either way.
struct NewCollectionExpr : cursiv::Extends<Expr> {
  TypeRef* type;
  vector<Expr*> keys;
  vector<Expr*> values;
};

struct CastExpr : cursiv::Extends<Expr> {
  TypeRef* type;
  Expr* operand;
};

struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct InstanceOfExpr : cursiv::Extends<Expr> {
  Expr* value;
  TypeRef* type;
};

struct TernaryExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

// `insert x;`, `update x;`, `delete x;`, `undelete x;` used as an
// expression, not only a statement.
struct DmlExpr : cursiv::Extends<Expr> {
  string_view op;
  string_view mode;  // `user` / `system` spelling after `as`, or empty
  Expr* value;
};

// `upsert x;`, `upsert x field;`: at most one field expression, the only
// shape the language actually uses.
struct UpsertExpr : cursiv::Extends<Expr> {
  string_view mode;
  Expr* value;
  Expr* field;
};

struct MergeExpr : cursiv::Extends<Expr> {
  string_view mode;
  Expr* master;
  Expr* duplicates;
};

// -- Initializers, declarators, parameters ----

struct Initializer : cursiv::Extends<cursiv::AstNode> {};
struct ExprInit : cursiv::Extends<Initializer> {
  Expr* value;
};
struct ArrayInit : cursiv::Extends<Initializer> {
  vector<Initializer*> items;
};

struct VarDeclarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  uint8 extraDims;  // legacy trailing `[]` after the name
  Initializer* init;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  boolean varargs;
  string_view name;
  uint8 extraDims;
};

// A class/interface body member or a block item: one shared base for every
// list that mixes declarations and statements freely.
struct Item : cursiv::Extends<cursiv::AstNode> {};

// -- Statements ----

struct Stmt : cursiv::Extends<Item> {};

struct Block : cursiv::Extends<Stmt> {
  vector<Item*> items;
};

struct LocalVarDecl : cursiv::Extends<Stmt> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  vector<VarDeclarator*> declarators;
};

// `expr;` and the empty statement `;`.
struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct DoStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

struct ForStmt : cursiv::Extends<Stmt> {
  LocalVarDecl* initDecl;
  vector<Expr*> initExprs;
  Expr* cond;
  vector<Expr*> update;
  Stmt* body;
};

struct ForEachStmt : cursiv::Extends<Stmt> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  string_view name;
  uint8 extraDims;
  Expr* iterable;
  Stmt* body;
};

struct SwitchWhen : cursiv::Extends<cursiv::AstNode> {
  boolean isElse;
  TypeRef* type;  // an sObject type binding, otherwise null
  string_view name;
  vector<Expr*> values;
  Block* body;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<SwitchWhen*> cases;
};

// `System.runAs(user) { ... }`.
struct RunAsStmt : cursiv::Extends<Stmt> {
  Expr* value;
  Stmt* body;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  vector<string_view> types;  // multi-catch: `catch (A | B e)`
  string_view name;
  Block* body;
};

// A declared resource: `init` is never null -- a try-with-resources
// declaration always requires one.
struct Resource : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  string_view name;
  uint8 extraDims;
  Expr* init;
};

struct TryStmt : cursiv::Extends<Stmt> {
  vector<Resource*> resources;
  Block* body;
  vector<CatchClause*> catches;
  Block* finallyBlock;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* body;
};

// -- Members ----

struct FieldDecl : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  vector<VarDeclarator*> declarators;
};

struct PropertyAccessor : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  boolean isSetter;
  Block* body;  // null: `;` body
};

struct PropertyDecl : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  TypeRef* type;
  vector<VarDeclarator*> declarators;
  vector<PropertyAccessor*> accessors;
};

struct MethodDecl : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  vector<TypeParam*> typeParams;
  TypeRef* returnType;  // null: void
  string_view name;
  vector<Param*> params;
  vector<string_view> throwsList;
  Block* body;  // null: abstract/interface (`;` body)
};

struct ConstructorDecl : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  vector<TypeParam*> typeParams;
  string_view name;
  vector<Param*> params;
  vector<string_view> throwsList;
  Block* body;
};

struct InitBlock : cursiv::Extends<Item> {
  boolean isStatic;
  Block* body;
};

struct TypeDecl : cursiv::Extends<Item> {};

struct ClassDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam*> typeParams;
  TypeRef* superclass;
  vector<TypeRef*> interfaces;
  vector<Item*> members;
};

struct InterfaceDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<TypeRef*> extendsList;
  vector<Item*> members;
};

struct EnumConstant : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Expr*> args;
  boolean hasBody;
  vector<Item*> classBody;
};

struct EnumDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeRef*> interfaces;
  vector<EnumConstant*> constants;
  vector<Item*> members;
};

// `@interface` element: a typed name with an empty parameter list and an
// optional default value. Real Apex has no custom annotations, but the
// grammar reaches this shape regardless.
struct AnnotationTypeElement : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  TypeRef* type;
  string_view name;
  AnnotationValue* defaultValue;
};

struct AnnotationTypeDecl : cursiv::Extends<TypeDecl> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<Item*> members;
};

// -- Top level ----

struct PackageDecl : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
};

struct ImportDecl : cursiv::Extends<cursiv::AstNode> {
  boolean isStatic;
  string_view qualifiedName;
  boolean onDemand;  // trailing `.*`
};

struct CompilationUnit : cursiv::Extends<cursiv::AstNode> {
  PackageDecl* package_;
  vector<ImportDecl*> imports;
  vector<TypeDecl*> types;
};

}  // namespace apex::ast
