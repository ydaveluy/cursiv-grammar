// The atl language: its AST. ATL transforms models with rules built from an
// embedded OCL expression language; `Expr` is that language's family, and
// `Type` extends it because ATL lets a bare type name stand as a value
// (`self.oclIsTypeOf(String)`).
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace atl::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

// A model slot of the transformation header (`IN: UML`), not a type.
struct OclModel : cursiv::Extends<cursiv::AstNode> {
  string_view alias;
  string_view modelName;
};

// A type as written in the language: a type annotation and a primary
// expression share this grammar, so `Type` sits in the `Expr` family instead
// of getting its own.
struct Type : cursiv::Extends<Expr> {};

// A class of a named model, `UML!Class`.
struct ModelType : cursiv::Extends<Type> {
  string_view modelName;
  string_view className;
};

struct AnyType : cursiv::Extends<Type> {};   // OclAny
struct MetaType : cursiv::Extends<Type> {};  // OclType, the type of a type

// Integer, Real, Boolean, String.
struct PrimType : cursiv::Extends<Type> {
  string_view keyword;
};

struct TupleAttr : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type;
};

struct TupleTypeRef : cursiv::Extends<Type> {  // TupleType(a: T, ...)
  vector<TupleAttr*> attrs;
};

struct MapTypeRef : cursiv::Extends<Type> {  // Map(K, V)
  Type* keyType;
  Type* valueType;
};

// Bag(T), Set(T), OrderedSet(T), Sequence(T), Collection(T): one class, the
// keyword tells them apart.
struct CollTypeRef : cursiv::Extends<Type> {
  string_view keyword;
  Type* elementType;
};

struct UndefinedLit : cursiv::Extends<Expr> {};  // OclUndefined
struct SuperExpr : cursiv::Extends<Expr> {};     // super

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct IntLit : cursiv::Extends<Expr> {
  int64 value;
};

struct FloatLit : cursiv::Extends<Expr> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view value;
};

struct EnumLit : cursiv::Extends<Expr> {  // '#' IDENTIFIER
  string_view name;
};

struct NameRef : cursiv::Extends<Expr> {  // a variable, iterator or `self`
  string_view name;
};

struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenExpr;
  Expr* elseExpr;
};

struct Feature : cursiv::Extends<cursiv::AstNode> {};

// A binding used by `let` (mandatory type), a tuple part (optional type), a
// rule's `using` block and the attribute form of a feature: one shape,
// `IDENTIFIER (':' oclType)? '=' expr`.
struct VarDecl : cursiv::Extends<Feature> {
  string_view name;
  Type* type;
  Expr* value;
};

struct LetExpr : cursiv::Extends<Expr> {
  VarDecl* binding;
  Expr* body;
};

// Bag{...}, Set{...}, OrderedSet{...}, Sequence{...}: the keyword is the
// kind, the shape is otherwise identical.
struct SetLit : cursiv::Extends<Expr> {
  string_view kind;
  vector<Expr*> items;
};

struct MapEntry : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
};

struct MapLit : cursiv::Extends<Expr> {
  vector<MapEntry*> entries;
};

struct TupleLit : cursiv::Extends<Expr> {
  vector<VarDecl*> parts;
};

// `a.b`, `a.b()`, `a->b()`: one postfix step nests the previous chain.
struct MethodCall : cursiv::Extends<Expr> {
  Expr* object;
  boolean arrow;  // '->' rather than '.'
  string_view name;
  vector<Expr*> args;
};

struct MemberAccess : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
};

// `->name(iter, ... | body)`: a named iterator expression (collect, select,
// exists...).
struct IteratorCall : cursiv::Extends<Expr> {
  Expr* object;
  string_view name;
  vector<string_view> iterators;
  Expr* body;
};

// `->iterate(iter, ... ; acc = init | body)`.
struct IterateCall : cursiv::Extends<Expr> {
  Expr* object;
  vector<string_view> iterators;
  VarDecl* accum;
  Expr* body;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  string_view op;
  Expr* right;
};

struct Parameter : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type;
};

// operation and attribute (oclFeature): a helper defines one of these.
struct Operation : cursiv::Extends<Feature> {
  string_view name;
  vector<Parameter*> params;
  Type* returnType;
  Expr* body;
};

// `attribute` (oclFeature) reuses VarDecl above: same shape, name : type =
// expr.
struct ModuleElement : cursiv::Extends<cursiv::AstNode> {};

struct Helper : cursiv::Extends<ModuleElement> {
  Type* context;     // 'context' oclType, optional
  Feature* feature;  // an Operation*, or a VarDecl* (the attribute form)
};

struct InPatternElem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type;
  vector<string_view> inModels;
};

struct InPattern : cursiv::Extends<cursiv::AstNode> {
  vector<InPatternElem*> elements;
  Expr* filter;  // the guard '(' oclExpression ')', optional
};

struct Binding : cursiv::Extends<cursiv::AstNode> {  // IDENTIFIER ('<-' | '<:=') expr
  string_view name;
  Expr* value;
  boolean isAssignment = false;
};

// A plain out-pattern element and a `distinct ... foreach` one: one class,
// told apart by `isForEach` -- the shared tail (mapsTo, bindings) is
// identical.
struct OutPatternElem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Type* type;
  boolean isForEach;
  string_view inModel;  // simple form: 'in' IDENTIFIER
  string_view iterVar;  // foreach form: the bound iterator
  Expr* source;         // foreach form: 'in' oclExpression
  string_view mapsTo;
  vector<Binding*> bindings;
};

struct OutPattern : cursiv::Extends<cursiv::AstNode> {
  vector<OutPatternElem*> elements;
};

struct Statement : cursiv::Extends<cursiv::AstNode> {};

struct ExprStmt : cursiv::Extends<Statement> {
  Expr* value;
};

struct BindStmt : cursiv::Extends<Statement> {
  Expr* target;
  Expr* value;
  boolean isAssignment = false;
};

struct IfStmt : cursiv::Extends<Statement> {
  Expr* cond;
  vector<Statement*> thenStmts;
  vector<Statement*> elseStmts;
  boolean hasElse = false;
};

struct ForStmt : cursiv::Extends<Statement> {
  string_view iterVar;
  Expr* source;
  vector<Statement*> body;
};

struct ActionBlock : cursiv::Extends<cursiv::AstNode> {
  vector<Statement*> statements;
};

struct TransRule : cursiv::Extends<ModuleElement> {};

// A lazy rule and an abstract/nodefault rule: one class, since their bodies
// are identical and only the modifier keywords they admit (unique/lazy vs.
// nodefault; abstract/refining are shared) differ.
struct MatchedRule : cursiv::Extends<TransRule> {
  vector<string_view> modifiers;
  boolean isUnique;
  boolean isLazy;
  boolean isNodefault;
  boolean isAbstract;
  boolean isRefining;
  string_view name;
  string_view extendsRule;  // kept as text: resolving it needs the module's
                             // rule scope, a semantic pass
  InPattern* inPattern;
  vector<VarDecl*> variables;
  OutPattern* outPattern;
  ActionBlock* action;
};

struct CalledRule : cursiv::Extends<TransRule> {
  boolean isEntrypoint;
  boolean isEndpoint;
  string_view name;
  vector<Parameter*> params;
  vector<VarDecl*> variables;
  OutPattern* outPattern;
  ActionBlock* action;
};

struct Module : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<OclModel*> targetModels;
  string_view mode;  // "refining" or "from"
  vector<OclModel*> sourceModels;
  vector<string_view> uses;
  vector<ModuleElement*> elements;
};

struct Library : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<string_view> uses;
  vector<Helper*> helpers;
};

struct Query : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* body;
  vector<string_view> uses;
  vector<Helper*> helpers;
};

}  // namespace atl::ast
