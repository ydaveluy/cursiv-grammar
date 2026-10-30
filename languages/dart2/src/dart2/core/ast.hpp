// The dart2 language: its AST. Interpolated strings are parsed as real
// expressions (ordered text and embedded Expr), so a nested brace or matching
// quote inside `${...}` still resolves.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace dart2::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Type : cursiv::Extends<cursiv::AstNode> {};
struct Element : cursiv::Extends<cursiv::AstNode> {};
struct Directive : cursiv::Extends<cursiv::AstNode> {};
struct Metadata;
// Every top-level declaration carries the `@...` annotations written before it,
// on the base, so one `append<>` at the head of the dispatching rule fills
// whichever declaration follows.
struct TopLevelDecl : cursiv::Extends<cursiv::AstNode> {
  vector<Metadata*> annotations;
};

// Every member carries the `@...` annotations written before it. The list
// lives on the base so one spliced `append<>` at the head of the dispatching
// rule fills whichever member follows.
struct Member : cursiv::Extends<cursiv::AstNode> {
  vector<Metadata*> annotations;
};

// -- Types ----

// `pkg.Foo<T>?` -- `qualifier` is the optional library prefix before the dot.
struct NamedType : cursiv::Extends<Type> {
  string_view qualifier;
  string_view name;
  vector<Type*> typeArguments;
  boolean nullable;
};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* bound;
};

struct ParamType : cursiv::Extends<cursiv::AstNode> {
  Type* type;
  string_view name;  // empty when unnamed
  boolean required;  // `required` on a named parameter type
};

struct ParamTypeList : cursiv::Extends<cursiv::AstNode> {
  vector<ParamType*> positional;
  vector<ParamType*> optionalPositional;  // `[...]`
  vector<ParamType*> named;               // `{...}`
};

// One `Function(...)` suffix of a (possibly curried) function type.
struct FunctionTypeSeg : cursiv::Extends<cursiv::AstNode> {
  vector<TypeParam*> typeParameters;
  ParamTypeList* params;
  boolean nullable;
};

struct FunctionType : cursiv::Extends<Type> {
  Type* returnType;  // absent when unwritten
  vector<FunctionTypeSeg*> segments;
};

// -- Formal parameters ----

struct FunctionTypedSuffix : cursiv::Extends<cursiv::AstNode> {
  vector<TypeParam*> typeParameters;
  struct ParamList* params;
  boolean nullable;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean isCovariant;
  boolean isRequired;  // `required` on a named parameter
  boolean isFinal;
  boolean isVar;
  Type* type;
  boolean isThis;  // `this.name` field-formal parameter
  boolean isSuper;
  string_view name;
  FunctionTypedSuffix* functionSuffix;  // set when this is function-typed
  Expr* defaultValue;
  vector<Metadata*> annotations;
};

struct ParamList : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> positional;
  vector<Param*> optionalPositional;  // `[...]`
  vector<Param*> named;               // `{...}`
};

// -- Expressions ----

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct ThisExpr : cursiv::Extends<Expr> {};
struct SuperExpr : cursiv::Extends<Expr> {};
struct NullLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

// String text remains source text (escapes included); interpolations are Expr
// nodes. Ordered parts preserve empty boundaries without requiring empty lexer
// tokens.
struct StringLit : cursiv::Extends<Expr> {
  vector<variant<string_view, Expr*>> parts;
  boolean raw;
  boolean multiline;
};

// Adjacent string literals: `'a' 'b'` is one string in Dart.
struct AdjacentStrings : cursiv::Extends<Expr> {
  vector<StringLit*> parts;
};

struct SymbolLit : cursiv::Extends<Expr> {
  string_view text;
};

struct ExprElement : cursiv::Extends<Element> {
  Expr* value;
};

struct MapElement : cursiv::Extends<Element> {
  Expr* key;
  Expr* value;
};

struct SpreadElement : cursiv::Extends<Element> {
  boolean nullAware;
  Expr* value;
};

struct IfElement : cursiv::Extends<Element> {
  Expr* cond;
  Element* thenElement;
  Element* elseElement;
};

struct ForElement;

struct ListLit : cursiv::Extends<Expr> {
  boolean isConst;
  vector<Type*> typeArguments;
  vector<Element*> elements;
};

// `{...}` -- a set or a map; ambiguous at the syntax level in Dart itself.
struct SetOrMapLit : cursiv::Extends<Expr> {
  boolean isConst;
  vector<Type*> typeArguments;
  vector<Element*> elements;
};

struct ConstructorRef : cursiv::Extends<cursiv::AstNode> {
  string_view qualifier;
  string_view typeName;
  vector<Type*> typeArguments;
  string_view constructorName;  // empty for the unnamed constructor
};

struct Argument : cursiv::Extends<cursiv::AstNode> {
  string_view name;  // empty for a positional argument
  Expr* value;
};

struct ArgumentList : cursiv::Extends<cursiv::AstNode> {
  vector<Argument*> items;
};

struct NewExpr : cursiv::Extends<Expr> {
  boolean isConst;
  ConstructorRef* ctor;
  ArgumentList* args;
};

// `Type<Args>.name(args)` without `new`/`const`.
struct ConstructorInvocationExpr : cursiv::Extends<Expr> {
  string_view qualifier;
  string_view typeName;
  vector<Type*> typeArguments;
  string_view constructorName;
  ArgumentList* args;
};

struct FunctionExpr : cursiv::Extends<Expr> {
  vector<TypeParam*> typeParameters;
  ParamList* params;
  boolean isAsync;
  boolean isGenerator;  // trailing `*` on async/sync
  boolean isSync;
  Expr* bodyExpr;  // `=> expr` form
  Stmt* bodyBlock;
};

struct AwaitExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct ThrowExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean nullAware;
  string_view property;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean nullAware;
  Expr* index;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Type*> typeArguments;
  ArgumentList* args;
};

// `x!` -- null assertion.
struct TypeApplyExpr : cursiv::Extends<Expr> {
  Expr* expression;
  vector<Type*> typeArguments;
};

struct NullAssertExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct IsTest : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
  Type* type;
};

struct AsExpr : cursiv::Extends<Expr> {
  Expr* operand;
  Type* type;
};

struct ConditionalExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
};

// One `..member`/`..[i]` step, with its own trailing selectors and an
// optional assignment -- the chain hangs off an implicit cascade receiver.
// A cascade step hangs off an implicit receiver, so it is spelled as its
// own small node rather than reusing MemberExpr/IndexExpr/CallExpr, which
// all expect a real `object`/`callee` operand.
struct CascadeStep : cursiv::Extends<cursiv::AstNode> {};

struct CascadeMemberStep : cursiv::Extends<CascadeStep> {
  string_view property;
};

struct CascadeIndexStep : cursiv::Extends<CascadeStep> {
  Expr* index;
};

struct CascadeCallStep : cursiv::Extends<CascadeStep> {
  vector<Type*> typeArguments;
  ArgumentList* args;
};

struct CascadeNullAssertStep : cursiv::Extends<CascadeStep> {};

struct CascadeSection : cursiv::Extends<cursiv::AstNode> {
  vector<CascadeStep*> steps;
  AssignExpr* assignment;
};

struct CascadeExpr : cursiv::Extends<Expr> {
  Expr* target;
  boolean nullAware;
  vector<CascadeSection*> sections;
};

// -- Statements ----

struct Label : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct LabelledStmt : cursiv::Extends<Stmt> {
  vector<Label*> labels;
  Stmt* body;
};

struct BlockStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> statements;
};

struct VarBinding : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* init;
};

struct VarSpec;

struct VarDeclStmt : cursiv::Extends<Stmt> {
  vector<Metadata*> annotations;
  VarSpec* spec;
};

struct FunctionDeclStmt : cursiv::Extends<Stmt> {
  vector<Metadata*> annotations;
  Type* returnType;
  string_view name;
  FunctionExpr* function;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct AssertStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Expr* message;
};

struct ForParts : cursiv::Extends<cursiv::AstNode> {};

struct ForClassicParts : cursiv::Extends<ForParts> {
  VarDeclStmt* init;
  Expr* initExpr;
  Expr* cond;
  vector<Expr*> update;
};

struct ForInParts : cursiv::Extends<ForParts> {
  boolean isFinal;
  boolean isVar;
  boolean isConst;
  Type* type;
  string_view name;
  Expr* iterable;
};

struct ForStmt : cursiv::Extends<Stmt> {
  boolean isAwait;
  ForParts* parts;
  Stmt* body;
};

struct ForElement : cursiv::Extends<Element> {
  boolean isAwait;
  ForParts* parts;
  Element* body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct DoStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

struct SwitchCase : cursiv::Extends<cursiv::AstNode> {
  vector<Label*> labels;
  Expr* value;  // null for `default`
  vector<Stmt*> body;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<SwitchCase*> cases;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct RethrowStmt : cursiv::Extends<Stmt> {};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  Type* onType;
  string_view exceptionName;
  string_view stackTraceName;
  BlockStmt* body;
};

struct TryStmt : cursiv::Extends<Stmt> {
  BlockStmt* body;
  vector<CatchClause*> catches;
  BlockStmt* finallyBlock;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct YieldStmt : cursiv::Extends<Stmt> {
  boolean isEach;
  Expr* value;
};

// -- Directives ----

struct LibraryName : cursiv::Extends<Directive> {
  vector<string_view> parts;
  vector<Metadata*> annotations;
};

struct Combinator : cursiv::Extends<cursiv::AstNode> {
  boolean isHide;
  vector<string_view> names;
};

struct ConfiguredUri : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> testName;
  StringLit* testValue;  // null when the `== value` part is absent
  StringLit* uri;
};

struct ImportDirective : cursiv::Extends<Directive> {
  StringLit* uri;
  vector<ConfiguredUri*> configurations;
  boolean isDeferred;
  string_view asName;
  vector<Combinator*> combinators;
  vector<Metadata*> annotations;
};

struct ExportDirective : cursiv::Extends<Directive> {
  StringLit* uri;
  vector<ConfiguredUri*> configurations;
  vector<Combinator*> combinators;
  vector<Metadata*> annotations;
};

struct PartDirective : cursiv::Extends<Directive> {
  StringLit* uri;
  vector<Metadata*> annotations;
};

struct PartOfDirective : cursiv::Extends<Directive> {
  vector<string_view> nameParts;  // set when `part of a.b.c;`
  StringLit* uri;                 // set when `part of 'uri.dart';`
  vector<Metadata*> annotations;
};

// -- Declarations ----

struct VarSpec : cursiv::Extends<cursiv::AstNode> {
  boolean isExternal;
  boolean isStatic;
  boolean isCovariant;
  boolean isLate;
  boolean isFinal;
  boolean isConst;
  boolean isVar;
  Type* type;
  vector<VarBinding*> vars;
};

struct FieldDecl : cursiv::Extends<Member> {
  VarSpec* spec;
};

struct Initializer : cursiv::Extends<cursiv::AstNode> {};

struct FieldInitializer : cursiv::Extends<Initializer> {
  string_view name;
  Expr* value;
};

struct SuperInitializer : cursiv::Extends<Initializer> {
  string_view constructorName;
  ArgumentList* args;
};

struct AssertInitializer : cursiv::Extends<Initializer> {
  Expr* cond;
  Expr* message;
};

struct Redirection : cursiv::Extends<cursiv::AstNode> {
  string_view constructorName;
  ArgumentList* args;
};

struct ConstructorDecl : cursiv::Extends<Member> {
  boolean isExternal;
  boolean isConst;
  boolean isFactory;
  string_view className;
  string_view name;
  ParamList* params;
  vector<Initializer*> initializers;
  Redirection* redirect;         // `: this(...)` / `: this.x(...)`
  ConstructorRef* factoryRedirect;  // `= Other.ctor`
  Stmt* body;
  Expr* bodyExpr;
};

enum class MemberKind : std::uint8_t { function, getter, setter, op };

struct MethodDecl : cursiv::Extends<Member> {
  boolean isExternal;
  boolean isStatic;
  boolean isAbstract;  // signature with no body
  MemberKind kind = MemberKind::function;
  Type* returnType;
  string_view name;  // for op: the operator's spelling
  vector<TypeParam*> typeParameters;  // a plain function only, never a getter/setter/operator
  ParamList* params;
  boolean isAsync;
  boolean isGenerator;
  boolean isSync;
  boolean isNative;
  StringLit* nativeName;
  Stmt* body;
  Expr* bodyExpr;
};

struct Metadata : cursiv::Extends<cursiv::AstNode> {
  vector<Type*> typeArguments;
  string_view qualifier;
  string_view name;
  string_view constructorName;
  ArgumentList* args;  // set for `@Ctor(args)`, absent for `@name`
};

struct ClassDecl : cursiv::Extends<TopLevelDecl> {
  boolean isAbstract;
  string_view name;
  vector<TypeParam*> typeParameters;
  Type* superclass;
  vector<Type*> mixins;
  vector<Type*> interfaces;
  vector<Member*> members;
  // `class A = B with C;` -- a mixin application class has no member list.
  Type* mixinApplication;
};

struct MixinDecl : cursiv::Extends<TopLevelDecl> {
  string_view name;
  vector<TypeParam*> typeParameters;
  vector<Type*> onTypes;
  vector<Type*> interfaces;
  vector<Member*> members;
};

struct ExtensionDecl : cursiv::Extends<TopLevelDecl> {
  string_view name;  // empty for an unnamed extension
  vector<TypeParam*> typeParameters;
  Type* onType;
  vector<Member*> members;
};

struct EnumEntry : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Metadata*> annotations;
  vector<Type*> typeArguments;
  string_view constructorName;
  ArgumentList* args;
};

struct EnumDecl : cursiv::Extends<TopLevelDecl> {
  string_view name;
  vector<TypeParam*> typeParameters;
  vector<Type*> mixins;
  vector<Type*> interfaces;
  vector<EnumEntry*> entries;
  vector<Member*> members;
};

struct TypedefDecl : cursiv::Extends<TopLevelDecl> {
  string_view name;
  vector<TypeParam*> typeParameters;
  Type* aliasedType;               // `typedef Name<T> = Type;`
  Type* functionReturnType;        // `typedef Name(params);` form
  ParamList* functionParams;
};

struct TopLevelVariable : cursiv::Extends<TopLevelDecl> {
  VarSpec* spec;
};

struct TopLevelFunction : cursiv::Extends<TopLevelDecl> {
  boolean isExternal;
  MemberKind kind = MemberKind::function;
  Type* returnType;
  string_view name;
  vector<TypeParam*> typeParameters;  // a plain function only
  ParamList* params;
  boolean isAsync;
  boolean isGenerator;
  boolean isSync;
  Stmt* body;
  Expr* bodyExpr;
};

// -- The document ----

struct CompilationUnit : cursiv::Extends<cursiv::AstNode> {
  string_view scriptTag;
  LibraryName* libraryName;
  vector<ImportDirective*> imports;
  vector<ExportDirective*> exports;
  vector<PartDirective*> parts;
  PartOfDirective* partOf;
  vector<TopLevelDecl*> declarations;
  Expr* exprEntry;  // set when the whole document is one bare expression
  Stmt* stmtEntry;  // set when the whole document is one bare statement
};

}  // namespace dart2::ast
