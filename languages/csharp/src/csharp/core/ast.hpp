// The csharp language: its AST. Syntactic, not resolved -- there is no
// symbol table, so a name that could be a type, a variable or a pattern
// target stays a name; a later pass decides which.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace csharp::ast {

// -- Types ----

struct Type;

struct TupleTypeElement : cursiv::Extends<cursiv::AstNode> {
  Type* typeRef;
  string_view name;
};

// One generic argument: a plain wrapper, so a generic type's own argument
// list does not point directly at its own class.
struct TypeArg : cursiv::Extends<cursiv::AstNode> {
  Type* typeRef;
};

// A type name: predefined keyword, `var`/`dynamic`, or a dotted path with
// per-name generic arguments. `tupleElements` non-empty makes
// this a tuple type instead.
struct TypeSegment : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TypeArg *> arguments;
};

struct Type : cursiv::Extends<cursiv::AstNode> {
  boolean functionPointer = false;
  string_view callingConvention;
  vector<string_view> callingConventions;
  vector<struct Param *> functionParameters;
  string_view alias;
  vector<TypeSegment *> segments;
  vector<TupleTypeElement*> tupleElements;
  vector<string_view> suffixes; // ?, *, []/[,] in their original order
};

// -- Shared pieces ----

struct Argument : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view mode;
  struct Expr* value;
};

struct Attribute : cursiv::Extends<cursiv::AstNode> {
  string_view target;
  Type* name;
  vector<Argument*> args;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attributes;
  vector<string_view> modifiers;
  Type* typeRef;
  string_view name;
  struct Expr* defaultValue;
  boolean isArglist = false;
};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attributes;
  string_view variance;
  string_view name;
};

struct TypeParamConstraintClause : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> keywordConstraints;
  vector<Type*> typeConstraints;
  boolean hasNew = false;
};

// `value` holds the initializer, an expression or an ArrayInitializerExpr.
struct Declarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean isRef = false;
  struct Expr* value;
};

// A designation: a name, a discard, or (`items` non-empty) a nested tuple --
// the tuple form is the distinct subclass `TupleDesignation` below, so a
// tuple's own items do not point directly at its own class.
struct Designation : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean discard = false;
  vector<Designation*> items;
};

struct TupleDesignation : cursiv::Extends<Designation> {};

// -- Patterns ----

struct Pattern : cursiv::Extends<cursiv::AstNode> {};

struct DeclarationPattern : cursiv::Extends<Pattern> {
  Type* typeRef;
  Designation* designation;
};

struct ConstantPattern : cursiv::Extends<Pattern> {
  struct Expr* value;
};

struct VarPattern : cursiv::Extends<Pattern> {
  Designation* designation;
};

struct Subpattern : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  struct Expr* expression;
  Pattern* value;
};

struct PositionalPattern : cursiv::Extends<Pattern> {
  Type* typeRef;
  vector<Subpattern*> items;
  vector<Subpattern*> propertyItems;
  Designation* designation;
};

struct PropertyPattern : cursiv::Extends<Pattern> {
  Type* typeRef;
  vector<Subpattern*> items;
  Designation* designation;
};

struct DiscardPattern : cursiv::Extends<Pattern> {};

struct TypePattern : cursiv::Extends<Pattern> {
  Type *typeRef;
};

struct UnaryPattern : cursiv::Extends<Pattern> {
  string_view op;
  Pattern *operand;
};

struct BinaryPattern : cursiv::Extends<Pattern> {
  Pattern *left;
  string_view op;
  Pattern *right;
};

struct RelationalPattern : cursiv::Extends<Pattern> {
  string_view op;
  struct Expr *value;
};

struct ListPattern : cursiv::Extends<Pattern> {
  vector<Pattern *> items;
  Designation *designation;
};

struct SlicePattern : cursiv::Extends<Pattern> {
  Pattern *pattern;
};

// -- Expressions ----

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct NameExpr : cursiv::Extends<Expr> {
  string_view alias;
  string_view name;
  vector<Type*> typeArguments;
};

// Raw text for every literal kind (number, char, string, bool, null): the
// shape of the text tells a reader which.
struct LiteralExpr : cursiv::Extends<Expr> {
  string_view text;
};

struct InterpolationHole : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  Expr* alignment;
  string_view format;
};

struct InterpolatedStringExpr : cursiv::Extends<Expr> {
  boolean verbatim = false;
  vector<string_view> texts;
  vector<InterpolationHole*> holes;
};

struct TupleExprElement : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct TupleExpr : cursiv::Extends<Expr> {
  vector<TupleExprElement*> elements;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view member;
  vector<Type*> typeArguments;
  boolean nullConditional = false;
  boolean pointerArrow = false;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* target;
  vector<Argument*> args;
  boolean nullConditional = false;
};

struct InvokeExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Argument*> args;
};

struct ThisExpr : cursiv::Extends<Expr> {};
struct BaseExpr : cursiv::Extends<Expr> {};

// Postfix `++`/`--`/`!` (null-forgiving).
struct PostfixExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view op;
};

// Prefix `+ - ! ~ ++ -- * & await`.
struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct CastExpr : cursiv::Extends<Expr> {
  Type* typeRef;
  Expr* operand;
};

struct RangeExpr : cursiv::Extends<Expr> {
  Expr* from;
  Expr* to;
};

struct SwitchArm : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
  Expr* guard;
  Expr* result;
};

struct SwitchExpr : cursiv::Extends<Expr> {
  Expr* value;
  vector<SwitchArm*> arms;
};

// Every binary operator, `??` and `is`/`as`'s left side included.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct IsExpr : cursiv::Extends<Expr> {
  Expr* value;
  Type* typeRef;
  Pattern* pattern;
};

struct AsExpr : cursiv::Extends<Expr> {
  Expr* value;
  Type* typeRef;
};

struct ThrowExpr : cursiv::Extends<Expr> {
  Expr* value;
};

// `type identifier`, e.g. `out int x` (Source: 12.19).
struct DeclarationExpr : cursiv::Extends<Expr> {
  Type* typeRef;
  string_view name;
};

struct ConditionalExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
  boolean refThen = false;
  boolean refElse = false;
};

struct LambdaExpr : cursiv::Extends<Expr> {
  boolean isAsync = false;
  boolean isStatic = false;
  vector<Attribute *> attributes;
  string_view refKind;
  Type *returnType;
  vector<Param*> params;
  struct Stmt* blockBody;
  Expr* exprBody;
  boolean refBody = false;
  Expr* refBodyValue;
};

struct AnonymousMethodExpr : cursiv::Extends<Expr> {
  boolean isAsync = false;
  boolean hasParamList = false;
  vector<Param*> params;
  struct Stmt* body;
};

struct Ordering : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  string_view direction;
};

// One clause of a query expression: `kind` is one of from/let/where/join/
// orderby/select/group/into -- the long tail of LINQ query syntax read
// generically rather than as one class per clause. A query continuation
// (`into`) opens a new segment in the same flat list rather than nesting.
struct QueryClause : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  Type* typeRef;
  string_view name;
  Expr* expr1;
  Expr* expr2;
  Expr* expr3;
  string_view intoName;
  vector<Ordering*> orderings;
};

struct QueryExpr : cursiv::Extends<Expr> {
  vector<QueryClause*> clauses;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Expr* value;
  boolean refValue = false;
};

// `{ }`-initializers: an object initializer's members, or (reused for
// collection/array/stackalloc initializers) a flat expression list.
struct ObjectInitMember : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Argument*> indexArgs;
  Expr* value;
};

struct ObjectInitializerExpr : cursiv::Extends<Expr> {
  vector<ObjectInitMember*> members;
};

struct ArrayInitializerExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct CollectionElement : cursiv::Extends<cursiv::AstNode> {
  boolean spread = false;
  Expr *value;
};

struct CollectionExpr : cursiv::Extends<Expr> {
  vector<CollectionElement *> elements;
};

struct WithExpr : cursiv::Extends<Expr> {
  Expr *target;
  ObjectInitializerExpr *init;
};

struct ObjectCreationExpr : cursiv::Extends<Expr> {
  Type* typeRef;
  vector<Argument*> args;
  Expr* init;
};

struct DelegateCreationExpr : cursiv::Extends<Expr> {
  Type* typeRef;
  Expr* arg;
};

struct MemberDeclarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct AnonymousObjectCreationExpr : cursiv::Extends<Expr> {
  vector<MemberDeclarator*> members;
};

struct ArrayCreationExpr : cursiv::Extends<Expr> {
  Type* elementType;
  vector<Expr*> sizes;
  vector<string_view> extraRanks;
  Expr* init;
};

struct TypeofExpr : cursiv::Extends<Expr> {
  Type* typeRef;
  string_view unboundText;
  boolean isVoid = false;
};

struct SizeofExpr : cursiv::Extends<Expr> {
  Type* typeRef;
};

struct CheckedExpr : cursiv::Extends<Expr> {
  Expr* value;
  boolean isChecked = true;
};

struct DefaultExpr : cursiv::Extends<Expr> {
  Type* typeRef;
};

// `nameof(...)`: the named entity kept as the raw text it spans.
struct NameofExpr : cursiv::Extends<Expr> {
  string_view text;
};

struct StackallocExpr : cursiv::Extends<Expr> {
  Type* elementType;
  Expr* size;
  Expr* init;
};

// `__makeref`/`__reftype`/`__refvalue` (MS extensions): one generic node.
struct MsExtensionExpr : cursiv::Extends<Expr> {
  string_view kind;
  Expr* arg1;
  Type* typeRef;
};

// -- Statements ----

struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct Block : cursiv::Extends<Stmt> {
  vector<Stmt*> statements;
};

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt* body;
};

struct LocalVarDeclStmt : cursiv::Extends<Stmt> {
  boolean isVar = false;
  string_view refKind;
  Type* typeRef;
  vector<Declarator*> declarators;
};

struct LocalConstDeclStmt : cursiv::Extends<Stmt> {
  Type* typeRef;
  vector<Declarator*> declarators;
};

struct LocalFunctionDeclStmt : cursiv::Extends<Stmt> {
  vector<string_view> modifiers;
  string_view refKind;
  Type* returnType;
  boolean isVoid = false;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<Param*> params;
  vector<TypeParamConstraintClause*> constraints;
  Stmt* body;
  Expr* exprBody;
  boolean isRefBody = false;
};

// `using` declaration (C# 8): the rest of the enclosing block is its scope.
struct UsingDeclStmt : cursiv::Extends<Stmt> {
  boolean isAwait = false;
  boolean isVar = false;
  Type* typeRef;
  vector<Declarator*> declarators;
  vector<Stmt*> statements;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct SwitchLabel : cursiv::Extends<cursiv::AstNode> {
  Pattern* pattern;
  Expr* guard;
  boolean isDefault = false;
};

struct SwitchSection : cursiv::Extends<cursiv::AstNode> {
  vector<SwitchLabel*> labels;
  vector<Stmt*> statements;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<SwitchSection*> sections;
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
  Stmt* initDecl;
  vector<Expr*> initExprs;
  Expr* cond;
  vector<Expr*> iterators;
  Stmt* body;
};

struct ForeachStmt : cursiv::Extends<Stmt> {
  boolean isAwait = false;
  string_view refKind;
  boolean isVar = false;
  Type* typeRef;
  string_view name;
  Designation* deconstruct;
  Expr* collection;
  Stmt* body;
};

struct BreakStmt : cursiv::Extends<Stmt> {};
struct ContinueStmt : cursiv::Extends<Stmt> {};

struct GotoStmt : cursiv::Extends<Stmt> {
  string_view label;
  Expr* caseValue;
  boolean isDefault = false;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
  boolean isRef = false;
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  Type* exceptionType;
  string_view name;
  Expr* filter;
  Stmt* body;
};

struct TryStmt : cursiv::Extends<Stmt> {
  Stmt* block;
  vector<CatchClause*> catches;
  Stmt* finallyBlock;
};

struct CheckedStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  boolean isChecked = true;
};

struct LockStmt : cursiv::Extends<Stmt> {
  Expr* value;
  Stmt* body;
};

struct UsingStmt : cursiv::Extends<Stmt> {
  boolean isAwait = false;
  Stmt* resourceDecl;
  Expr* resourceExpr;
  Stmt* body;
};

struct YieldStmt : cursiv::Extends<Stmt> {
  Expr* value;
  boolean isBreak = false;
};

struct UnsafeStmt : cursiv::Extends<Stmt> {
  Stmt* body;
};

struct FixedStmt : cursiv::Extends<Stmt> {
  Type* typeRef;
  vector<Declarator*> declarators;
  Stmt* body;
};

// -- Members and type declarations ----

// Base of every namespace- and type-body-level member: a namespace, a type
// declaration, or (inside a body) a field/method/property/... declaration.
struct Member : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attributes;
  vector<string_view> modifiers;
};

struct FieldDecl : cursiv::Extends<Member> {
  Type* typeRef;
  vector<Declarator*> declarators;
};

struct ConstDecl : cursiv::Extends<Member> {
  Type* typeRef;
  vector<Declarator*> declarators;
};

struct Accessor : cursiv::Extends<cursiv::AstNode> {
  string_view kind;
  vector<Attribute*> attributes;
  vector<string_view> modifiers;
  Stmt* body;
  Expr* exprBody;
  boolean isRef = false;
  Expr* refValue;
};

struct PropertyDecl : cursiv::Extends<Member> {
  string_view refKind;
  Type* typeRef;
  string_view name;
  Type* explicitInterface;
  vector<Accessor*> accessors;
  Expr* exprBody;
  Expr* initializer;
};

struct IndexerDecl : cursiv::Extends<Member> {
  string_view refKind;
  Type* typeRef;
  Type* explicitInterface;
  vector<Param*> params;
  vector<Accessor*> accessors;
  Expr* exprBody;
};

struct EventDecl : cursiv::Extends<Member> {
  Type* typeRef;
  vector<Declarator*> declarators;
  string_view name;
  Type* explicitInterface;
  vector<Accessor*> accessors;
};

struct MethodDecl : cursiv::Extends<Member> {
  string_view refKind;
  Type* returnType;
  boolean isVoid = false;
  Type* explicitInterface;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<Param*> params;
  vector<TypeParamConstraintClause*> constraints;
  Stmt* body;
  Expr* exprBody;
  boolean isRefBody = false;
};

struct OperatorDecl : cursiv::Extends<Member> {
  Type* returnType;
  boolean isChecked = false;
  string_view opKind;
  string_view op;
  vector<Param*> params;
  Stmt* body;
  Expr* exprBody;
};

struct ConstructorDecl : cursiv::Extends<Member> {
  string_view name;
  vector<Param*> params;
  string_view initializerKind;
  vector<Argument*> initializerArgs;
  Stmt* body;
  Expr* exprBody;
};

struct StaticConstructorDecl : cursiv::Extends<Member> {
  string_view name;
  Stmt* body;
  Expr* exprBody;
};

struct FinalizerDecl : cursiv::Extends<Member> {
  string_view name;
  Stmt* body;
  Expr* exprBody;
};

struct FixedSizeBufferDecl : cursiv::Extends<Member> {
  Type* elementType;
  vector<Declarator*> declarators;
};

struct ClassDecl : cursiv::Extends<Member> {
  boolean isRecord = false;
  string_view name;
  vector<Param *> primaryParams;
  vector<Argument *> baseArgs;
  vector<TypeParam*> typeParams;
  vector<Type*> bases;
  vector<TypeParamConstraintClause*> constraints;
  vector<Member*> members;
};

struct StructDecl : cursiv::Extends<Member> {
  boolean isRecord = false;
  string_view name;
  vector<Param *> primaryParams;
  vector<Argument *> baseArgs;
  vector<TypeParam*> typeParams;
  vector<Type*> bases;
  vector<TypeParamConstraintClause*> constraints;
  vector<Member*> members;
};

struct InterfaceDecl : cursiv::Extends<Member> {
  string_view name;
  vector<TypeParam*> typeParams;
  vector<Type*> bases;
  vector<TypeParamConstraintClause*> constraints;
  vector<Member*> members;
};

struct EnumMember : cursiv::Extends<cursiv::AstNode> {
  vector<Attribute*> attributes;
  string_view name;
  Expr* value;
};

struct EnumDecl : cursiv::Extends<Member> {
  string_view name;
  Type* baseType;
  vector<EnumMember*> members;
};

struct DelegateDecl : cursiv::Extends<Member> {
  string_view refKind;
  Type* returnType;
  boolean isVoid = false;
  string_view name;
  vector<TypeParam*> typeParams;
  vector<Param*> params;
  vector<TypeParamConstraintClause*> constraints;
};

struct ExternAlias : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct UsingDirective : cursiv::Extends<cursiv::AstNode> {
  boolean isGlobal = false;
  boolean isUnsafe = false;
  string_view kind;
  string_view alias;
  Type* target;
};

struct NamespaceDecl : cursiv::Extends<Member> {
  boolean fileScoped = false;
  vector<string_view> path;
  vector<ExternAlias*> externAliases;
  vector<UsingDirective*> usings;
  vector<Member*> members;
};

struct CompilationUnit : cursiv::Extends<cursiv::AstNode> {
  vector<ExternAlias*> externAliases;
  vector<UsingDirective*> usings;
  vector<Attribute*> globalAttributes;
  vector<Member*> members;
  vector<Stmt *> statements;
};

}  // namespace csharp::ast
