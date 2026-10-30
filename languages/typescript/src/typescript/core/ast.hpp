// The typescript language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace typescript::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Type : cursiv::Extends<cursiv::AstNode> {};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  boolean isConst;  // `<const T>`
  boolean isIn;     // `<in T>`
  boolean isOut;    // `<out T>`
  string_view name;
  Type* constraint;
  Type* defaultType;
};

// One node for every parameter list the grammar has (function, method,
// arrow, call/construct signature, function type): they differ only in
// incidental structuring, never in what one parameter can carry.
struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> decorators;
  boolean rest;
  string_view accessibility;  // "public" | "private" | "protected" | empty
  boolean isOverride;
  boolean isReadonly;
  Expr* pattern;               // Ident, ArrayLit or ObjectLit
  boolean optional;
  Type* typeAnnotation;
  Expr* defaultValue;
};

// -- Types ----

struct PredefinedType : cursiv::Extends<Type> {
  boolean unique;
  string_view name;  // a keyword ("any", "number", ...) or a literal type's text
};

struct ThisType : cursiv::Extends<Type> {};

struct TypeRef : cursiv::Extends<Type> {
  string_view name;  // dotted, as written
  vector<Type*> typeArgs;
};

struct TypeQuery : cursiv::Extends<Type> {
  string_view name;  // dotted, after `typeof`
  vector<Type*> typeArgs;
};

struct KeyOfType : cursiv::Extends<Type> {
  Type* operand;
};

// `readonly T[]`
struct ReadonlyType : cursiv::Extends<Type> {
  Type* operand;
};

// `{ readonly [K in C as N]?: T }`: `readonlyMod` and `optionalMod` keep the
// spelling, sign included (`-readonly`, `+?`), empty when absent.
struct MappedType : cursiv::Extends<Type> {
  string_view readonlyMod;
  string_view name;
  Type* constraint;
  Type* nameType;  // the `as` clause
  string_view optionalMod;
  Type* type;
};

// `x is Foo`, `this is Foo`, `asserts x is Foo`, `asserts x` (type null)
struct TypePredicateType : cursiv::Extends<Type> {
  boolean asserts;
  Type* param;  // a TypeRef or a ThisType
  Type* type;
};

// `A extends B ? C : D`
struct ConditionalType : cursiv::Extends<Type> {
  Type* checkType;
  Type* extendsType;
  Type* trueType;
  Type* falseType;
};

// `infer U`, `infer U extends C`
struct InferType : cursiv::Extends<Type> {
  TypeParam* typeParam;
};

// `a${T}b`: `quasis.size() == types.size() + 1`.
struct TemplateLiteralType : cursiv::Extends<Type> {
  vector<string_view> quasis;
  vector<Type*> types;
};

// `typeof import("m").A.B<T>` / `import("m")`
struct ImportType : cursiv::Extends<Type> {
  boolean isTypeOf;
  Type* argument;
  Expr* options;  // `{ with: { ... } }`
  string_view qualifier;  // dotted, after the closing parenthesis
  vector<Type*> typeArgs;
};

// `T[]` (index null) or `T[K]` (indexed access).
struct IndexedType : cursiv::Extends<Type> {
  Type* object;
  Type* index;
};

struct TupleType : cursiv::Extends<Type> {
  vector<Type*> elements;
};

// `[a: string]`, `[a?: string]`, `[...a: string[]]`
struct NamedTupleMember : cursiv::Extends<Type> {
  boolean rest;
  string_view name;
  boolean optional;
  Type* type;
};

// `[string?]`
struct OptionalType : cursiv::Extends<Type> {
  Type* type;
};

// `[...string[]]`
struct RestType : cursiv::Extends<Type> {
  Type* type;
};

// `|`, `&`
struct TypeBinExpr : cursiv::Extends<Type> {
  Type* left;
  string_view op;
  Type* right;
};

// One node for every interface/type-literal member -- a property, a call
// signature, a construct signature, an index signature, and a method:
// which fields are set says which one.
struct TypeMember : cursiv::Extends<cursiv::AstNode> {
  boolean readonly_;
  boolean isGetter;
  boolean isSetter;
  boolean isCall;
  boolean isConstruct;
  boolean isIndex;
  boolean isMethod;
  boolean computed;
  boolean optional;
  Expr* key;              // null for a call/construct/index signature
  string_view indexName;  // index signature's bracketed parameter name
  Type* indexKeyType;     // index signature's key type: string, a pattern...
  vector<TypeParam*> typeParams;
  vector<Param*> params;  // call/construct/method signatures
  Type* type;             // property type, or a signature's return type
};

struct ObjectType : cursiv::Extends<Type> {
  vector<TypeMember*> members;
};

struct FunctionType : cursiv::Extends<Type> {
  vector<TypeParam*> typeParams;
  vector<Param*> params;
  Type* returnType;
};

struct ConstructorType : cursiv::Extends<Type> {
  boolean isAbstract;
  vector<TypeParam*> typeParams;
  vector<Param*> params;
  Type* returnType;
};

// -- Expressions ---- A binding target (a declarator's name, a parameter, a
// catch binding, the left side of a for-in/for-of) has no grammar of its own,
// same as the javascript port: it is an `Expr*` -- an `Ident`, an `ArrayLit`,
// or an `ObjectLit` -- and whether it is really a valid pattern is left to a
// later pass.

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct ThisExpr : cursiv::Extends<Expr> {};
struct SuperExpr : cursiv::Extends<Expr> {};

struct NullLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// The spelling, not the value: a numeric literal's meaning depends on
// separators and suffixes a parser should not resolve.
struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct BigIntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text;  // quotes included, escapes raw
};

struct RegexLit : cursiv::Extends<Expr> {
  string_view text;  // slashes and flags included
};

// Untagged when `tag` is null. `quasis.size() == expressions.size() + 1`.
struct TemplateLit : cursiv::Extends<Expr> {
  Expr* tag;
  vector<string_view> quasis;
  vector<Expr*> expressions;
};

// `...x`: an array/argument element, never a property (an object spread
// sets `Prop::key` to null instead).
struct Spread : cursiv::Extends<Expr> {
  Expr* argument;
};

struct Elision : cursiv::Extends<Expr> {};
struct InstantiationExpr : cursiv::Extends<Expr> {
  Expr* expression;
  vector<Type*> typeArgs;
};
struct ArrayLit : cursiv::Extends<Expr> {
  vector<Expr*> elements;  // elisions have their own Elision node
};

struct Prop : cursiv::Extends<cursiv::AstNode> {
  boolean computed;
  boolean isGetter;
  boolean isSetter;
  boolean isMethod;
  boolean isAsync;
  boolean isGenerator;
  // Both null for `...expr` (spread) and for the permissive `{expr}`
  // shorthand also allowed there: tell them apart by whether `value` is a
  // `Spread`.
  Expr* key;    // null for a spread or shorthand property
  Expr* value;  // FunctionExpr* for get/set/method; the argument when key is null
};

struct ObjectLit : cursiv::Extends<Expr> {
  vector<Prop*> properties;
};

struct BlockStmt;

struct FunctionExpr : cursiv::Extends<Expr> {
  boolean isAsync;
  boolean isGenerator;
  string_view name;  // empty when anonymous
  vector<TypeParam*> typeParams;
  vector<Param*> params;
  Type* returnType;
  BlockStmt* body;  // null: a signature with no body (an overload)
};

struct ArrowFunctionExpr : cursiv::Extends<Expr> {
  boolean isAsync;
  vector<TypeParam*> typeParams;
  vector<Param*> params;
  Type* returnType;
  BlockStmt* block;  // exactly one of block/exprBody is set
  Expr* exprBody;
};

struct ClassMember;
struct AbstractDecl;

// No `decorators` here: a decorator list applies to a class declaration,
// never to the expression form.
struct ClassExpr : cursiv::Extends<Expr> {
  boolean isAbstract;
  string_view name;  // empty when anonymous
  vector<TypeParam*> typeParams;
  Type* superClass;   // a TypeRef, from `extends typeReference`
  Expr* superExpr;    // `extends mixin(Base)`: an expression that is no TypeRef
  vector<Type*> implementsClause;
  vector<ClassMember*> members;
};

// One member, tagged rather than subclassed. A field is what is left once
// the getter/setter/method/staticBlock/abstractDecl tags are ruled out.
// Unlike `Prop`, a method's signature lives directly on the member (typeParams/
// params/returnType/body) rather than behind a nested FunctionExpr: a class
// method carries far more than an object-literal one (typeParameters,
// accessibility, abstract, a body-less overload), and nothing here adopts a
// bare function value the way a property's shorthand does.
struct ClassMember : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> decorators;
  boolean isStatic;
  boolean isAbstract;
  boolean isReadonly;
  boolean isDeclare;
  boolean isOverride;
  boolean isAccessor;
  string_view accessibility;
  boolean isGetter;
  boolean isSetter;
  boolean isMethod;
  boolean isAsync;
  boolean isGenerator;
  boolean isIndex;       // `[key: string]: T`
  boolean computed;
  boolean optional;
  boolean definite;  // `x!: T`
  string_view indexName;  // index signature's bracketed parameter name
  Type* indexKeyType;     // index signature's key type
  Expr* key;  // null for a static block, an index signature or an abstract form
  Type* typeAnnotation;  // a field's type
  vector<TypeParam*> typeParams;  // method/getter/setter
  vector<Param*> params;          // method/getter/setter
  Type* returnType;               // method/getter/setter
  Expr* value;   // a field's initializer; null for a method/getter/setter/block
  BlockStmt* body;  // method/getter/setter/static-block body; null for a
                     // field or a body-less overload
  AbstractDecl* abstractDecl;  // the `abstract name(...): T` / `abstract` field form
};

struct NewExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Type*> typeArgs;
  vector<Expr*> args;  // `new X` and `new X()` are both an empty list
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Type*> typeArgs;  // generic calls on any callee, including optional calls
  vector<Expr*> args;
  boolean optional;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  Expr* property;  // Ident* unless computed; its name keeps a leading '#'
  boolean computed;
  boolean optional;
  vector<Type*> typeArgs;  // legacy field; generic arguments now belong to CallExpr/InstantiationExpr
};

// `e!`, the non-null assertion operator.
struct NonNullExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

// `++x`, `--x`. A separate class from the postfix form and not a flag on it:
// the two compute different values, and a directive can set a member from a
// matched token but not tag a whole alternative for free.
struct PrefixUpdateExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct PostfixUpdateExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct AwaitExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct YieldExpr : cursiv::Extends<Expr> {
  boolean delegate;  // yield*
  Expr* argument;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct ConditionalExpr : cursiv::Extends<Expr> {
  Expr* test;
  Expr* consequent;
  Expr* alternate;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

// `expr as Type`.
struct AsExpr : cursiv::Extends<Expr> {
  Expr* expression;
  Type* target;
};

// `expr satisfies Type`.
struct SatisfiesExpr : cursiv::Extends<Expr> {
  Expr* expression;
  Type* target;
};

// `new.target`, `import.meta`.
struct MetaProperty : cursiv::Extends<Expr> {
  string_view meta;      // "new" | "import"
  string_view property;  // "target" | "meta"
};

// `import(source, options)`
struct ImportCallExpr : cursiv::Extends<Expr> {
  Expr* source;
  Expr* options;
};

// The old-style cast `<Type>expr`.
struct TypeAssertionExpr : cursiv::Extends<Expr> {
  vector<Type*> typeArgs;
  Expr* expression;
};

// -- Statements ----

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct BlockStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* expression;
};

struct Declarator : cursiv::Extends<cursiv::AstNode> {
  Expr* id;  // Ident, ArrayLit or ObjectLit
  boolean optional;  // `abstract x?: T`
  boolean definite;  // `let x!: T`
  Type* typeAnnotation;
  Expr* init;
};

struct VarDecl : cursiv::Extends<Stmt> {
  string_view kind;  // "var" | "let" | "const" | "using" | empty
  boolean isAwaitUsing;  // independent of a surrounding `for await`
  boolean hasTrailingComma;  // resource list syntax, diagnosed after parsing
  boolean isDeclare;
  boolean isReadonly;
  string_view accessibility;
  vector<Declarator*> declarations;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* test;
  Stmt* consequent;
  Stmt* alternate;
};

struct DoWhileStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* test;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* test;
  Stmt* body;
};

struct ForStmt : cursiv::Extends<Stmt> {
  VarDecl* initDecl;
  Expr* initExpr;
  Expr* test;
  Expr* update;
  Stmt* body;
};

struct ForInStmt : cursiv::Extends<Stmt> {
  VarDecl* leftDecl;
  Expr* leftExpr;
  Expr* right;
  Stmt* body;
};

struct ForOfStmt : cursiv::Extends<Stmt> {
  boolean isAwait;
  VarDecl* leftDecl;
  Expr* leftExpr;
  Expr* right;
  Type* asType;  // optional `as Type` on the iteration variable
  Stmt* body;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* argument;
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr* argument;
};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* body;
};

struct WithStmt : cursiv::Extends<Stmt> {
  Expr* object;
  Stmt* body;
};

struct SwitchCase : cursiv::Extends<cursiv::AstNode> {
  Expr* test;  // null for `default`
  vector<Stmt*> consequent;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* discriminant;
  vector<SwitchCase*> cases;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  Expr* param;  // null: catch with no binding
  Type* typeAnnotation;
  BlockStmt* body;
};

struct TryStmt : cursiv::Extends<Stmt> {
  BlockStmt* block;
  CatchClause* handler;
  BlockStmt* finalizer;
};

struct DebuggerStmt : cursiv::Extends<Stmt> {};

struct FunctionDecl : cursiv::Extends<Stmt> {
  boolean isDeclare;
  boolean isAsync;
  boolean isGenerator;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<Param*> params;
  Type* returnType;
  BlockStmt* body;  // null: an overload signature
};

struct ClassDecl : cursiv::Extends<Stmt> {
  vector<Expr*> decorators;
  boolean isDeclare;
  boolean isAbstract;
  string_view name;
  vector<TypeParam*> typeParams;
  Type* superClass;  // a TypeRef, from `extends typeReference`
  Expr* superExpr;   // `extends mixin(Base)`: an expression that is no TypeRef
  vector<Type*> implementsClause;
  vector<ClassMember*> members;
};

// `interface I<T> extends A, B { ... }`
struct InterfaceDecl : cursiv::Extends<Stmt> {
  boolean isDeclare;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<Type*> extendsClause;
  vector<TypeMember*> members;
};

// `type Name<T> = ...`
struct TypeAliasDecl : cursiv::Extends<Stmt> {
  boolean isDeclare;
  string_view name;
  vector<TypeParam*> typeParams;
  Type* type;
};

struct EnumMember : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;  // null when the member has no initializer
};

struct EnumDecl : cursiv::Extends<Stmt> {
  boolean isDeclare;
  boolean isConst;
  string_view name;
  vector<EnumMember*> members;
};

// `namespace N.M { }`, `module M { }`, `declare module "m" { }`, `declare
// global { }`; a shorthand ambient module `declare module "m";` has no body.
struct NamespaceDecl : cursiv::Extends<Stmt> {
  boolean isDeclare;
  string_view kind;  // "namespace" | "module" | "global"
  string_view name;  // dotted, or the module's string literal; empty for global
  vector<Stmt*> body;
};

// `export as namespace N;`
struct NamespaceExportDecl : cursiv::Extends<Stmt> {
  string_view name;
};

// `import a = require("m");`, `import a = N.M;` (`export import ...` is an
// ExportStmt around it).
struct ImportEqualsDecl : cursiv::Extends<Stmt> {
  boolean isTypeOnly;
  string_view name;
  string_view source;   // the `require("...")` string, quotes included
  string_view entity;   // the dotted name of the other form
};

// `abstract name(...): T;` (a method signature), `abstract get name(): T;` /
// `abstract set name(v: T);` (an accessor signature) or `abstract let x: T;` (a
// variable declaration) -- the `abstract` shapes, reused both as a statement
// and, via `ClassMember::abstractDecl`, as a class member.
struct AbstractDecl : cursiv::Extends<Stmt> {
  boolean isGetter;
  boolean isSetter;
  string_view name;  // the method and accessor forms; empty otherwise
  vector<TypeParam*> typeParams;
  vector<Param*> params;
  Type* returnType;
  VarDecl* declaration;  // the variable-declaration form; null for the method form
};

// The default specifier and a namespace specifier both leave `imported`
// empty (nothing is written between `import` and `from`); `isNamespace`
// tells them apart, a plain named specifier always has `imported` set.
struct ImportSpecifier : cursiv::Extends<cursiv::AstNode> {
  boolean isTypeOnly;  // `{ type X }`
  boolean isNamespace;
  string_view imported;
  string_view local;
};

// `with { type: "json" }`
struct ImportAttribute : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  string_view value;
};

struct ImportStmt : cursiv::Extends<Stmt> {
  boolean isTypeOnly;  // import type ...
  vector<ImportSpecifier*> specifiers;
  string_view source;
  vector<ImportAttribute*> attributes;
};

struct ExportSpecifier : cursiv::Extends<cursiv::AstNode> {
  boolean isTypeOnly;  // `{ type X }`
  string_view exported;
  string_view local;
};

struct ExportStmt : cursiv::Extends<Stmt> {
  vector<Expr*> decorators;  // those written before `export`
  boolean isTypeOnly;  // export type { ... }, export type * from ...
  boolean isDefault;
  boolean isEquals;     // export = <expr>
  boolean isNamespace;  // export * from "..."
  vector<ExportSpecifier*> specifiers;
  Stmt* declaration;   // export <VarDecl|ClassDecl|FunctionDecl|InterfaceDecl|...>
  Expr* defaultExpr;   // `export default <expr>` and `export = <expr>`
  string_view source;  // re-export "from" clause, empty otherwise
  vector<ImportAttribute*> attributes;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  string_view prologue;  // a byte-order mark and a `#!` line, as written
  vector<Stmt*> body;
};

}  // namespace typescript::ast
