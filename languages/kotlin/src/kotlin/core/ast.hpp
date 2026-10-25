// The kotlin language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace kotlin::ast {

// Forward declarations: the grammar is mutually recursive (a type names a
// parameter, a parameter's default is an expression, an expression's lambda
// holds statements, a statement is a declaration or an expression...).

struct Node;
struct Declaration;
struct Expr;
struct Type;

struct Annotation;
struct Argument;

struct UserType;
struct TypeSegment;
struct TypeProjection;
struct NullableType;
struct FunctionType;
struct DynamicType;
struct ParenthesizedType;

struct Param;
struct ClassParam;
struct VariableDeclarator;
struct TypeParam;
struct TypeConstraint;
struct LambdaParam;

struct SuperType;
struct SuperTypeEntry;
struct SuperTypeCall;
struct SuperTypeDelegate;

struct ClassDecl;
struct PrimaryConstructor;
struct SecondaryConstructor;
struct EnumEntry;
struct ObjectDecl;
struct CompanionObjectDecl;
struct FunctionDecl;
struct PropertyDecl;
struct Getter;
struct Setter;
struct ExplicitBackingField;
struct TypeAliasDecl;

struct Block;
struct Ident;
struct Literal;
struct StringLiteral;
struct StringPart;
struct FunctionLiteral;
struct ThisExpr;
struct SuperExpr;
struct IfExpr;
struct WhenExpr;
struct WhenEntry;
struct TryExpr;
struct CatchClause;
struct ObjectLiteral;
struct CollectionLiteral;
struct ThrowExpr;
struct ReturnExpr;
struct BreakExpr;
struct ContinueExpr;
struct ForExpr;
struct WhileExpr;
struct DoWhileExpr;
struct CallableReferenceExpr;
struct PrefixExpr;
struct AnnotatedExpr;
struct LabeledExpr;
struct PostfixExpr;
struct CallExpr;
struct IndexExpr;
struct MemberExpr;
struct BinaryExpr;
struct CastExpr;
struct TypeTestExpr;

struct InitBlock;
struct ImportHeader;
struct KotlinFile;

// Anything that may stand as a top-level member or a block statement: a
// declaration or an expression, both directly (Kotlin's `if`/`when`/`try`/
// loops are expressions, not their own statement kinds).
struct Node : cursiv::Extends<cursiv::AstNode> {};

struct Declaration : cursiv::Extends<Node> {};

// Every expression kind's family. The builder's node-family check runs
// against this base, so any expression fits any `Expr*` slot.
struct Expr : cursiv::Extends<Node> {};

// Every type kind's family. `annotations`/`isSuspend` are the modifiers a
// `type` rule may carry regardless of which alternative it resolves to.
struct Type : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  boolean isSuspend;
};

struct Annotation : cursiv::Extends<cursiv::AstNode> {
  string_view useSiteTarget;
  string_view name;
  vector<Argument*> arguments;
  vector<TypeProjection *> typeArguments;
};

struct AnnotationGroup : cursiv::Extends<Annotation> {
  vector<Annotation *> annotations;
};

// A call argument, and an annotation's: `name = *expr`.
struct Argument : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean spread;
  Expr *value;
};

struct TypeSegment : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<TypeProjection*> typeArguments;
};

// One entry of a type argument list: a type, a variance-annotated type, or
// the bare `*` star projection.
struct TypeProjection : cursiv::Extends<cursiv::AstNode> {
  string_view variance;
  boolean isStar;
  Type* type;
};

// `Foo.Bar<X>.Baz` -- a dotted path, each segment with its own arguments.
struct UserType : cursiv::Extends<Type> {
  vector<TypeSegment*> segments;
};

// `T?` -- multiple `?` in a row (the source grammar allows the run) collapse
// to one wrapping.
struct NullableType : cursiv::Extends<Type> {
  Type* base;
};

// `(Receiver.)? (params) -> Ret`. A parameter here may be bare (name empty).
struct FunctionType : cursiv::Extends<Type> {
  vector<Param *> contextParameters;
  Type* receiver;
  vector<Param*> parameters;
  Type* returnType;
};

struct DynamicType : cursiv::Extends<Type> {};

// `suspend (T)`, `@A (T)`: modifiers on a parenthesized type, which the type
// inside cannot take.
struct ParenthesizedType : cursiv::Extends<Type> {
  Type* type;
};

struct IntersectionType : cursiv::Extends<Type> {
  Type *left;
  Type *right;
};

struct UnionType : cursiv::Extends<Type> {
  Type *left;
  Type *right;
};

// A function/lambda/setter parameter: `vararg @A name: Type = default`.
struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  string_view name;
  Type* type;
  Expr* defaultValue;
};

// A primary constructor parameter: as `Param`, plus the `val`/`var` that
// promotes it to a property.
struct ClassParam : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  string_view valOrVar;
  string_view name;
  Type* type;
  Expr* defaultValue;
};

struct VariableDeclarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type;
};

struct TypeParam : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  string_view name;
  boolean isStar;
  Type* bound;
};

struct TypeConstraint : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  Type* bound;
};

// A lambda parameter: a plain `name: Type` or a destructured `(a, b): Type`
// -- `declarators` holds one entry for the plain form, several for the
// destructured one.
struct LambdaParam : cursiv::Extends<cursiv::AstNode> {
  vector<VariableDeclarator*> declarators;
  Type* type;
};

// A class/object supertype: a bare interface, a constructor call, or a `by`
// delegate -- `delegationSpecifier`'s three readings, one base so a
// supertype list can hold any mix.
struct SuperType : cursiv::Extends<Node> {
  vector<Annotation*> annotations;
};

struct SuperTypeEntry : cursiv::Extends<SuperType> {
  Type* type;
};

struct SuperTypeCall : cursiv::Extends<SuperType> {
  Type* type;
  vector<TypeProjection*> typeArguments;
  vector<Argument*> arguments;
  vector<Expr*> trailingLambdas;
};

struct SuperTypeDelegate : cursiv::Extends<SuperType> {
  Type* type;
  Expr* delegate;
};

struct PrimaryConstructor : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  vector<ClassParam*> params;
};

struct EnumEntry : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  vector<Argument*> arguments;
  vector<Node*> body;
};

// `class`/`interface` in one node (`isInterface` tells them apart); `enum`
// is a modifier, so `enumEntries` is simply empty where the source has none.
struct ClassDecl : cursiv::Extends<Declaration> {
  vector<Param *> contextParameters;
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  boolean isInterface;
  string_view name;
  vector<TypeParam*> typeParameters;
  PrimaryConstructor* primaryCtor;
  vector<SuperType*> superTypes;
  vector<TypeConstraint*> typeConstraints;
  vector<EnumEntry*> enumEntries;
  vector<Node*> members;
};

struct SecondaryConstructor : cursiv::Extends<Declaration> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  vector<Param*> params;
  string_view delegationKind;
  vector<Argument*> delegationArgs;
  Block* body;
};

struct ObjectDecl : cursiv::Extends<Declaration> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  string_view name;
  PrimaryConstructor* primaryCtor;
  vector<SuperType*> superTypes;
  vector<Node*> members;
};

struct CompanionObjectDecl : cursiv::Extends<Declaration> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  string_view name;
  vector<SuperType*> superTypes;
  vector<Node*> members;
};

struct FunctionDecl : cursiv::Extends<Declaration> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  vector<Param*> contextParameters;
  vector<TypeParam*> typeParameters;
  Type* receiverType;
  string_view name;
  vector<Param*> valueParameters;
  Type* returnType;
  vector<TypeConstraint*> typeConstraints;
  Node* body;
};

struct Getter : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  Type* returnType;
  Node* body;
};

struct Setter : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  Param* param;
  Node* body;
};

struct ExplicitBackingField : cursiv::Extends<cursiv::AstNode> {
  Type* type;
  Expr* initializer;
};

struct PropertyDecl : cursiv::Extends<Declaration> {
  vector<Param *> contextParameters;
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  boolean isVar;
  vector<TypeParam*> typeParameters;
  Type* receiverType;
  vector<VariableDeclarator*> declarators;
  vector<TypeConstraint*> typeConstraints;
  boolean isDelegate;
  Expr* initializer;
  Getter* getter;
  Setter* setter;
  ExplicitBackingField* backingField;
};

struct InitBlock : cursiv::Extends<Node> {
  Block* body;
};

struct TypeAliasDecl : cursiv::Extends<Declaration> {
  vector<string_view> modifiers;
  vector<Annotation*> annotations;
  string_view name;
  vector<TypeParam*> typeParameters;
  Type* type;
};

struct Block : cursiv::Extends<Node> {
  vector<Node*> statements;
};

// A name used as a value; `hasValPrefix` is the rare `val x` reading of
// `atomicExpression`.
struct Ident : cursiv::Extends<Expr> {
  string_view name;
  boolean hasValPrefix;
};

// Every non-string literal (bool, integer, hex, binary, char, real, long,
// null), kept as written like every other literal in this AST -- its shape
// tells a reader the kind.
struct Literal : cursiv::Extends<Expr> {
  string_view text;
};

struct StringPart : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  Expr* expr;
};

struct StringLiteral : cursiv::Extends<Expr> {
  boolean multiline;
  vector<StringPart*> parts;
};

struct FunctionLiteral : cursiv::Extends<Expr> {
  vector<Annotation *> annotations;
  vector<LambdaParam*> parameters;
  vector<Node*> body;
};

struct ThisExpr : cursiv::Extends<Expr> {
  string_view label;
};

struct SuperExpr : cursiv::Extends<Expr> {
  Type* targetType;
  string_view label;
};

// `controlStructureBody` (a block or a bare expression) is a `Node*` either
// way, since `Block` and every expression extend it.
struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Node* thenBranch;
  Node* elseBranch;
};

// A branch's conditions: a plain expression, or `in`/`is` reused from the
// binary-expression and type-test nodes with their implicit left operand
// left null.
struct WhenEntry : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> conditions;
  boolean isElse;
  Expr *guard;
  Node* body;
};

struct WhenExpr : cursiv::Extends<Expr> {
  Expr* subject;
  PropertyDecl *subjectDeclaration;
  vector<WhenEntry*> entries;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
  string_view name;
  Type* type;
  Block* body;
};

struct TryExpr : cursiv::Extends<Expr> {
  Block* tryBlock;
  vector<CatchClause*> catches;
  Block* finallyBlock;
};

struct ObjectLiteral : cursiv::Extends<Expr> {
  vector<SuperType*> superTypes;
  vector<Node*> members;
};

struct CollectionLiteral : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct ThrowExpr : cursiv::Extends<Expr> {
  Expr* value;
};

struct ReturnExpr : cursiv::Extends<Expr> {
  string_view label;
  Expr* value;
};

struct BreakExpr : cursiv::Extends<Expr> {
  string_view label;
};

struct ContinueExpr : cursiv::Extends<Expr> {
  string_view label;
};

struct ForExpr : cursiv::Extends<Expr> {
  vector<Annotation *> annotations;
  vector<VariableDeclarator*> declarators;
  Expr* iterable;
  Node* body;
};

struct WhileExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Node* body;
};

struct DoWhileExpr : cursiv::Extends<Expr> {
  Node* body;
  Expr* cond;
};

// `Type?::name`, `Type::class`, `this::class`.
struct CallableReferenceExpr : cursiv::Extends<Expr> {
  Type* receiverType;
  boolean isThisReceiver;
  boolean safe;
  string_view name;
  boolean isClass;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct AnnotatedExpr : cursiv::Extends<Expr> {
  vector<Annotation*> annotations;
  Expr* operand;
};

struct LabeledExpr : cursiv::Extends<Expr> {
  string_view label;
  Expr* operand;
};

// Postfix `++`/`--`/`!!`.
struct PostfixExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view op;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<TypeProjection*> typeArguments;
  vector<Argument*> arguments;
  vector<Expr*> trailingLambdas;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  vector<Expr*> indices;
};

// `member` holds a plain name, the overwhelmingly common case; `memberExpr`
// holds the source grammar's looser reading (a literal, a parenthesized
// expression, `if`/`when`/`try`, a lambda -- anything a primary can start)
// when the name alone does not match.
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean safe;
  string_view member;
  Expr* memberExpr;
};

// Every binary operator, `op` holding its spelling verbatim (`+`, `to`, an
// arbitrary infix function name, an assignment...): the set is open-ended
// (a user-defined infix function), so text rather than an enum.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

// `operand as Type` / `operand as? Type` / `operand : Type` -- a type on the
// right, unlike every other operator, so its own node rather than
// `BinaryExpr`.
struct CastExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view op;
  Type* type;
};

// `operand is Type` / `operand !is Type`; `operand` is left null when built
// from a `when` branch's bare `is Type` condition.
struct TypeTestExpr : cursiv::Extends<Expr> {
  Expr* operand;
  boolean negated;
  Type* type;
};

struct ImportHeader : cursiv::Extends<cursiv::AstNode> {
  string_view path;
  boolean wildcard;
  string_view alias;
};

struct KotlinFile : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> fileAnnotations;
  string_view packageName;
  vector<ImportHeader*> imports;
  vector<Node*> members;
};

}  // namespace kotlin::ast
