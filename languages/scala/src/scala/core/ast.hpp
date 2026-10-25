// The scala language: its AST. Syntactic only, no symbol table: names are
// text, not `reference<>`, since imports and types mostly name things
// outside the document.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace scala::ast {

struct Type : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Pattern : cursiv::Extends<cursiv::AstNode> {};
struct Item : cursiv::Extends<cursiv::AstNode> {};  // a top/template/block statement

// A bare expression used as a statement (`blockStat`'s/`templateStat`'s
// trailing `expr`/`expr1` alternative) -- `Expr` is not itself an `Item`,
// since a type position needs the narrower distinction elsewhere.
struct ExprStmt : cursiv::Extends<Item> {
  Expr* value;
};

// A dotted path, optionally starting at `this`/`super`: `a.b.C`, `this.x`,
// `a.super[T].x`. `root` is the identifier right after this/super, or the
// whole first segment of a plain dotted chain; `segments` holds what follows.
struct StableId : cursiv::Extends<cursiv::AstNode> {
  string_view qualifierPrefix;  // the optional leading "Id." before this/super
  boolean hasThis;
  boolean hasSuper;
  string_view superQualifier;  // classQualifier's Id, empty unless super[Q]
  string_view root;
  vector<string_view> segments;
};

// -- Literals (shared between expressions and patterns) -------------------

struct IntLit : cursiv::Extends<Expr> {
  boolean negative;
  string_view text;
};
struct FloatLit : cursiv::Extends<Expr> {
  boolean negative;
  string_view text;
};
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};
struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};
struct StringLit : cursiv::Extends<Expr> {
  boolean multiline;
  string_view text;
};
struct InterpolationPart : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  Expr *expression;
  Pattern *pattern;
};
struct InterpolatedString : cursiv::Extends<Expr> {
  string_view prefix;
  boolean multiline;
  vector<InterpolationPart *> parts;
};
struct InterpolatedPattern : cursiv::Extends<Pattern> {
  string_view prefix;
  boolean multiline;
  vector<InterpolationPart *> parts;
};
struct SymbolLit : cursiv::Extends<Expr> {
  string_view text;
};
struct NullLit : cursiv::Extends<Expr> {};

// -- Types ------------------------------------------------------------

struct TemplateBody;
struct TypeParam;
struct Dcl;

// A parameter/result type shape shared by params, funDcl and functionArgTypes:
// `T`, `=> T` (by-name), `T*` (repeated).
struct ParamType : cursiv::Extends<cursiv::AstNode> {
  boolean byName;
  boolean repeated;
  Type* type;
};

// `T => U`. `params` is one element for a bare `T`, or the parenthesised list.
struct FunctionType : cursiv::Extends<Type> {
  vector<ParamType*> params;
  Type* result;
};

// `T forSome { type X; val y: Int }`.
struct ExistentialType : cursiv::Extends<Type> {
  Type* base;
  vector<Dcl*> decls;
};

// `T with U`, `A Op B` (infix type-level operator): one shared shape for both
// chains, `op` empty for the `with` case.
struct BinaryType : cursiv::Extends<Type> {
  Type* left;
  string_view op;
  Type* right;
};

// `T { def f: Int }`, or a bare `{ ... }` structural type when `base` is null.
struct RefinedType : cursiv::Extends<Type> {
  Type* base;
  vector<Item*> members;
};

struct Annotation;

// `T @ann` -- only built when at least one annotation follows a type.
struct AnnotatedType : cursiv::Extends<Type> {
  Type* base;
  vector<Annotation*> annotations;
};

// `stableId`, `stableId.type`, `(T, U)` (tuple), and the postfix chain
// `T[U]` / `T#X` folded onto the same node via `many`.
struct NamedType : cursiv::Extends<Type> {
  StableId* path;
  boolean isSingleton;  // `path.type`
};
struct TupleType : cursiv::Extends<Type> {
  vector<Type*> items;
};
struct AppliedType : cursiv::Extends<Type> {
  Type* base;
  vector<Type*> args;
};
struct TypeProjection : cursiv::Extends<Type> {
  Type* base;
  string_view member;
};
// `_ >: L <: U` in a type argument.
struct WildcardType : cursiv::Extends<Type> {
  Type* lowerBound;
  Type* upperBound;
};

// -- Type parameters, params, bindings ---------------------------------

// `+A[X] >: L <: U <% V1 <% V2 : C1 : C2` and a method's plain `[A]`, which
// leaves `variance` empty and `viewBounds`/`contextBounds` unused.
struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view variance;  // "+", "-" or empty
  boolean isUnderscore;
  string_view name;
  vector<TypeParam*> nestedParams;
  Type* lowerBound;
  Type* upperBound;
  vector<Type*> viewBounds;
  vector<Type*> contextBounds;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  ParamType* type;
  Expr* defaultValue;
};

struct ClassParam : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  boolean isVal;
  boolean isVar;
  string_view name;
  ParamType* type;
  Expr* defaultValue;
};

// One parameter clause `(...)` of a curried signature -- `vector<vector<T>>`
// is not a supported member shape, so a signature's `paramClauses` is a list
// of these instead.
struct ParamClause : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> params;
};
struct ClassParamClause : cursiv::Extends<cursiv::AstNode> {
  vector<ClassParam*> params;
};

// `(x, y: Int)`, and the bare `x` / `implicit x` / `_` forms of a lambda
// header, which synthesize a one-element list of this.
struct Binding : cursiv::Extends<cursiv::AstNode> {
  boolean isImplicit;
  boolean isUnderscore;
  string_view name;
  Type* type;
};

// -- Annotations, access -------------------------------------------------

// One curried argument list -- `vector<vector<T>>` is not a supported member
// shape, so a repeated `argumentExprs` (`@Ann(a)(b)`, `extends C(a)(b)`)
// appends one node of this per group instead.
struct ArgList : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> args;
  boolean varargsSplice;
};

// `type` is `Type*`, not `NamedType*`: an annotation's type shares the
// production ordinary types use, which also admits a tuple `(A, B)` --
// nothing here rules that out syntactically, though no real annotation
// writes one.
struct Annotation : cursiv::Extends<cursiv::AstNode> {
  Type* type;
  vector<ArgList*> argLists;
};

// -- Expressions ----------------------------------------------------------

struct CaseClause;
struct ClassTemplate;

struct Block : cursiv::Extends<Expr> {
  vector<Item*> stats;
  Expr* result;
};

struct MatchBlock : cursiv::Extends<Expr> {
  vector<CaseClause*> cases;
};

// `new C(...)` / `new { ... }`: exactly one of the two is set.
struct NewExpr : cursiv::Extends<Expr> {
  ClassTemplate* classTemplate;
  TemplateBody* templateBody;
};

struct PathExpr : cursiv::Extends<Expr> {
  StableId* path;
};

struct UnderscoreExpr : cursiv::Extends<Expr> {};

// `()`, `(a)` (grouping), `(a, b)` (tuple) -- told apart by `items.size()`.
struct ParenExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

// `x _`: a method turned into its function value.
struct EtaExpr : cursiv::Extends<Expr> {
  Expr* operand;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

struct TypeApplyExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Type*> typeArgs;
};

// `f(a, b)`, `f { ... }` (single block argument) and the trailing `xs: _*`
// splice marker.
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
  boolean varargsSplice;
};

struct PrefixOpExpr : cursiv::Extends<Expr> {
  string_view op;  // -, +, ~, !
  Expr* operand;
};

struct PostfixExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view postfixOp;
};

struct InfixExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct AscribeExpr : cursiv::Extends<Expr> {
  Expr* value;
  Type* type;
  vector<Annotation*> annotations;
  boolean varargsSplice;  // `: _*`
};

struct MatchExpr : cursiv::Extends<Expr> {
  Expr* value;
  vector<CaseClause*> cases;
};

struct Binding;

// `(bindings | implicit? Id | _) => body`; `params` synthesizes a one-element
// list for the bare-name and underscore forms.
struct Lambda : cursiv::Extends<Expr> {
  vector<Binding*> params;
  Expr* body;
};

struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenBranch;
  Expr* elseBranch;
};

struct WhileExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* body;
};

struct DoWhileExpr : cursiv::Extends<Expr> {
  Expr* body;
  Expr* cond;
};

struct TryExpr : cursiv::Extends<Expr> {
  Expr* body;
  Expr* catchHandler;
  Expr* finallyBlock;
};

struct Generator;
struct ForExpr : cursiv::Extends<Expr> {
  vector<Generator*> enumerators;
  boolean yields;
  Expr* body;
};

struct ThrowExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct ReturnExpr : cursiv::Extends<Expr> {
  Expr* value;
};

// `x = v`, `a.b = v`, `a(i) = v` -- `target` is the receiver expression built
// by the ordinary chain, so all three forms share one shape.
struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  Expr* value;
};

// -- for-comprehensions ----------------------------------------------------

struct Guard : cursiv::Extends<cursiv::AstNode> {
  Expr* cond;
};
struct ValueDef : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
  Expr* value;
};
struct Generator : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
  Expr* source;
  vector<cursiv::AstNode*> refinements;  // Guard* or ValueDef*
};

// -- Case clauses -----------------------------------------------------

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
  Expr* guard;
  Block* body;
};

// -- Patterns ---------------------------------------------------------

struct AltPattern : cursiv::Extends<Pattern> {
  Pattern* first;
  vector<Pattern*> rest;
};

struct TypedPattern : cursiv::Extends<Pattern> {
  boolean isUnderscore;
  string_view name;
  Type* type;
};

struct BindPattern : cursiv::Extends<Pattern> {
  string_view name;
  Pattern* inner;
};

struct InfixPattern : cursiv::Extends<Pattern> {
  Pattern* left;
  string_view op;
  Pattern* right;
};

struct WildcardPattern : cursiv::Extends<Pattern> {};

struct VarPattern : cursiv::Extends<Pattern> {
  string_view name;
};

struct LiteralPattern : cursiv::Extends<Pattern> {
  Expr* value;
};

struct StablePattern : cursiv::Extends<Pattern> {
  StableId* path;
};

// `C(a, b)`, `C(a, b, rest @ _*)`, `C(_*)`.
struct ExtractorPattern : cursiv::Extends<Pattern> {
  StableId* path;
  vector<Pattern*> args;
  boolean varargsSplice;
  string_view varargsBindName;
};

struct TuplePattern : cursiv::Extends<Pattern> {
  vector<Pattern*> items;
  boolean varargsSplice;
};

// -- Declarations and definitions --------------------------------------

// The `(annotation NL?)* modifier*` prefix belongs to the statement, not
// to the declaration alternative it precedes, but a `Dcl` node has nowhere
// else to keep it.
struct Dcl : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
};

struct ValDcl : cursiv::Extends<Dcl> {
  boolean isVar;
  vector<string_view> names;
  Type* type;
};

struct DefDcl : cursiv::Extends<Dcl> {
  string_view name;
  vector<TypeParam*> typeParams;
  vector<ParamClause*> paramClauses;
  vector<Param*> implicitParams;
  Type* returnType;
};

struct TypeDcl : cursiv::Extends<Dcl> {
  string_view name;
  vector<TypeParam*> typeParams;
  Type* lowerBound;
  Type* upperBound;
};

// `val`/`var` pattern definition: `patDef` shared by both, `isVar` tells them
// apart since they build identically. `uninitialized` is `var x: T = _` --
// the one var form with no expression, its names folded into `patterns` as
// bind patterns so it needs no separate class.
struct PatDef : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  boolean isVar;
  boolean uninitialized;
  vector<Pattern*> patterns;
  Type* type;
  Expr* value;
};

struct DefDef : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<ParamClause*> paramClauses;
  vector<Param*> implicitParams;
  Type* returnType;
  boolean isMacro;  // `= macro expr`
  Expr* value;   // `= expr`
  Block* body;   // `{ block }`
};

// `def this(...) = ...` / `{ ... }` -- a secondary constructor.
struct SecondaryCtorDef : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  vector<Param*> firstParams;
  vector<ParamClause*> moreParamClauses;
  vector<Param*> implicitParams;
  Expr* selfInvocation;  // the leading `this(...)` of constrExpr/constrBlock
  Block* body;           // set only for the constrBlock form
};

struct TypeDef : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam*> typeParams;
  Type* value;
};

// -- Templates ----------------------------------------------------------

struct SelfType : cursiv::Extends<cursiv::AstNode> {
  boolean isThis;
  string_view name;
  Type* type;
};

struct TemplateBody : cursiv::Extends<cursiv::AstNode> {
  SelfType* self;
  vector<Item*> members;
};

// The parents + optional body of a class/trait/object: `earlyDefs? parents
// templateBody?`, or a bare/extends-only `templateBody`. `firstParent` (with
// its curried constructor argument lists) is null for a trait or a bodyless
// definition; `withParents` holds the rest.
struct ClassTemplate : cursiv::Extends<cursiv::AstNode> {
  vector<PatDef*> earlyDefs;
  Type* firstParent;
  vector<ArgList*> constrArgLists;
  vector<Type*> withParents;
  SelfType* self;
  vector<Item*> members;
  boolean hasBody;
};

// `ctorModifiers` holds the constructor's own access modifier and, when it
// carries a bracket (`private[this]`), the bracket's content as one more
// entry -- kept flat rather than a separate qualifier field, since it is
// written at most once per definition.
struct ClassDef : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  boolean isCase;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<Annotation*> ctorAnnotations;
  vector<string_view> ctorModifiers;
  vector<ClassParamClause*> paramClauses;
  vector<ClassParam*> implicitParams;
  ClassTemplate* templateOpt;
};

struct TraitDef : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  string_view name;
  vector<TypeParam*> typeParams;
  ClassTemplate* templateOpt;
};

struct ObjectDef : cursiv::Extends<Item> {
  vector<Annotation*> annotations;
  vector<string_view> modifiers;
  boolean isCase;
  boolean isPackageObject;
  string_view name;
  ClassTemplate* templateOpt;
};

// -- Imports and packaging ----------------------------------------------

struct ImportSelector : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean renameToUnderscore;
  string_view renameTo;
};

// `a.b.{c, d => e}` / `a.b._` / `a.b.c` / `a.b` -- exactly one of
// `wildcard`, `singleName` or a non-empty `selectors` extends `path`.
struct ImportExpr : cursiv::Extends<cursiv::AstNode> {
  StableId* path;
  boolean wildcard;
  string_view singleName;
  vector<ImportSelector*> selectors;
};

struct Import : cursiv::Extends<Item> {
  vector<ImportExpr*> clauses;
};

struct QualId : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> parts;
};

struct Packaging : cursiv::Extends<Item> {
  QualId* name;
  vector<Item*> items;
};

struct CompilationUnit : cursiv::Extends<cursiv::AstNode> {
  vector<QualId*> packageClauses;
  vector<Item*> items;
};

}  // namespace scala::ast
