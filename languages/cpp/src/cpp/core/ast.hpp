// The cpp language: its AST. No symbol table: like the c port this one
// started from, a name is syntactic (Ident/QualifiedName), and the
// declaration/expression and functional-cast/call ambiguities resolve by
// ordered choice, not by knowing what a name denotes.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace cpp::ast {

// Anything a list of items may hold: a declaration, a statement, a function
// definition, an access-specifier label. The grammar keeps a function
// definition out of a block, not this type.

struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<Item> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};
// Anything name-shaped: an identifier, a qualified or templated name, an
// operator/conversion/literal-operator function name, a destructor name. A
// Name is also an Expr -- a bare name is itself a primary expression.
struct Name : cursiv::Extends<Expr> {};
// The type-denoting core of a declaration's specifier list.
struct TypeSpec : cursiv::Extends<cursiv::AstNode> {};

struct TypeId;
struct Declarator;
struct Specifiers;
struct CompoundStmt;
struct Param;
struct ExceptionSpec;

struct Ident : cursiv::Extends<Name> {
  string_view name;
};

// `Foo<int, T*>`: `args` holds each argument, a type read before an
// expression the same way a cast tries a type first everywhere else in
// this grammar.
struct TemplateId : cursiv::Extends<Name> {
  Name* templateName;
  vector<variant<TypeId*, Expr*>> args;
};

// `~Widget`, `~decltype(e)`, and the qualified forms `p->A::~A()`,
// `p->N::A::~A()` -- the latter's extra `N::A::` layer collapses into
// `qualifier`, itself already recursive.
struct DestructorName : cursiv::Extends<Name> {
  cursiv::AstNode* qualifier;  // Name* or a decltype TypeSpec*, or null
  Name* extraType;             // the redundant `(type ::)?` layer, or null
  cursiv::AstNode* target;     // Name* or a decltype TypeSpec*
};

// `operator+`, `operator new[]`, `operator()`: the spelling, so a caller does
// not have to know the operator set.
struct OperatorFunctionName : cursiv::Extends<Name> {
  string_view op;
};

// `operator TYPE`, a user-defined conversion function name.
struct ConversionFunctionName : cursiv::Extends<Name> {
  TypeId* type;
};

// `operator"" _km`, `operator "" _km`: the identifier suffix alone --
// `operator` and the (always empty, by the standard) string are fixed.
struct LiteralOperatorName : cursiv::Extends<Name> {
  string_view suffix;
};

// `N::T`, `::T`, `A<int>::B`: a left-recursive chain. `qualifier == nullptr`
// means a bare leading `::`; `id == nullptr` means this node is only a
// prefix, with nothing after it yet (mid-chain).
struct QualifiedName : cursiv::Extends<Name> {
  cursiv::AstNode* qualifier;  // Name* or a decltype TypeSpec*, or null
  boolean templateKw;
  Name* id;
};

// -- Literals and primaries ----

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};
struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};
struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};
// Adjacent string/user-defined-string literals concatenate, same as C; every
// other literal kind stands alone.
struct StringLit : cursiv::Extends<Expr> {
  vector<string_view> pieces;
};
struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};
struct NullptrLit : cursiv::Extends<Expr> {};
struct UserDefinedLit : cursiv::Extends<Expr> {
  string_view text;
};
struct ThisExpr : cursiv::Extends<Expr> {};

// One entry of a lambda-introducer's capture-list: `x`, `&x`, `this`,
// `*this`, or an init-capture `x = expr`.
struct Capture : cursiv::Extends<cursiv::AstNode> {
  boolean pack;
  boolean byRef;
  boolean isThis;      // `this` or `*this`
  string_view name;    // empty for `this`/`*this`
  Expr* init;          // set only for an init-capture
};

struct LambdaExpr : cursiv::Extends<Expr> {
  vector<Capture*> captures;
  string_view captureDefault;  // "&", "=", or empty
  boolean hasDeclarator;       // a lambdaDeclarator was written at all
  vector<Param*> params;
  boolean variadic;
  boolean mutableKw;
  ExceptionSpec* exceptionSpec;
  TypeId* trailingReturnType;
  CompoundStmt* body;
};

// `f(a, b)`: the callee is an expression, not a name -- `(*fp)(x)` is a call
// too.
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Expr*> args;
};
struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};
// `s.f`, `p->f`, `p->~T()`: one class, `arrow` written by the `->` token;
// `member` is a Name (Ident, QualifiedName, TemplateId, DestructorName...).
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  boolean arrow;
  Name* member;
};
struct PostfixExpr : cursiv::Extends<Expr> {
  Expr* operand;
  string_view op;  // "++" or "--"
};
// `dynamic_cast`/`static_cast`/`reinterpret_cast`/`const_cast<T>(e)`: one
// class, `kind` the keyword used.
struct NamedCastExpr : cursiv::Extends<Expr> {
  string_view kind;
  TypeId* type;
  Expr* operand;
};
// `typeid(e)`/`typeid(T)`: exactly one of `expr`, `type` is set, the
// expression reading tried first (a type can itself be a bare name, so
// trying the type reading first would swallow it).
struct TypeidExpr : cursiv::Extends<Expr> {
  Expr* expr;
  TypeId* type;
};
// `int(x)`, `MyClass(a, b)`, `int{5}`: a functional-style cast/construction.
// A plain-identifier type here is indistinguishable from a call: the
// primary-expression reading is tried first, so `Foo(x)` reads as a call,
// matching CallExpr.
struct FunctionalCastExpr : cursiv::Extends<Expr> {
  TypeId* type;
  vector<Expr*> args;
  boolean braced;
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};
struct SizeofExpr : cursiv::Extends<Expr> {
  Expr* operand;
};
struct SizeofTypeExpr : cursiv::Extends<Expr> {
  TypeId* type;
};
struct SizeofPackExpr : cursiv::Extends<Expr> {
  string_view name;
};
struct AlignofExpr : cursiv::Extends<Expr> {
  TypeId* type;
};
struct NoexceptExpr : cursiv::Extends<Expr> {
  Expr* expr;
};

struct NewExpr : cursiv::Extends<Expr> {
  boolean globalScope;
  vector<Expr*> placement;  // the placement-new argument list; empty if absent
  TypeId* type;
  boolean hasInit;
  vector<Expr*> initArgs;  // the initializer's argument list; empty if none/braced-empty
  boolean braced;
};
struct DeleteExpr : cursiv::Extends<Expr> {
  boolean globalScope;
  boolean isArray;
  Expr* operand;
};

// `(T)e`: the C-style cast this grammar keeps alongside the named casts.
struct CastExpr : cursiv::Extends<Expr> {
  TypeId* type;
  Expr* operand;
};

// Every binary level of the operator ladder (pointer-to-member through
// logical-or) and the comma operator, one more left-associative level over
// assignment. `op` holds the operator's text.
struct BinaryExpr : cursiv::Extends<Expr> {
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
struct ThrowExpr : cursiv::Extends<Expr> {
  Expr* value;  // null: a re-throw `throw;`
};

// `{ a, b, c }`: a brace-init-list. Extends Expr, since C++ accepts one
// wherever an initializer or an argument may stand (`return {1, 2};`).
struct PackExpansionExpr : cursiv::Extends<Expr> {
  Expr* pattern;
};

struct BracedInitList : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

// -- Type specifiers ----

// `unsigned long int`, `void`, `bool`: a list, since C++ spells one type with
// several keywords whose order carries nothing.
struct BuiltinType : cursiv::Extends<TypeSpec> {
  vector<string_view> keywords;
};
// A named type: possibly qualified, possibly a template-id -- `name` is an
// Ident, QualifiedName or TemplateId.
struct NamedType : cursiv::Extends<TypeSpec> {
  Name* name;
};
// `class`/`struct`/`union`/`enum` used as an elaborated-type-specifier
// (`struct S`, `enum E`), without a body.
struct ElaboratedType : cursiv::Extends<TypeSpec> {
  string_view keyword;
  Name* name;
};
// `typename N::T`: the `typename` disambiguator carries no information once
// parsed.
struct TypenameType : cursiv::Extends<TypeSpec> {
  Name* name;
};
struct DecltypeType : cursiv::Extends<TypeSpec> {
  Expr* expr;      // null when isAuto
  boolean isAuto;  // `decltype(auto)`
};

struct BaseSpecifier : cursiv::Extends<cursiv::AstNode> {
  string_view accessSpec;  // "public"/"protected"/"private", or empty
  boolean isVirtual;
  cursiv::AstNode* type;  // Name* or a decltype TypeSpec*
  boolean pack;
};

struct ClassType : cursiv::Extends<TypeSpec> {
  string_view keyword;  // "class", "struct" or "union"
  Name* name;            // null for an anonymous class/struct/union
  boolean isFinal;
  vector<BaseSpecifier*> bases;
  boolean defined;  // a `{ ... }` body was present
  vector<Item*> members;
};

// `public:`/`protected:`/`private:` inside a class body, kept in `members`
// in source order rather than grouping declarations under it.
struct AccessLabel : cursiv::Extends<Item> {
  string_view keyword;
};

struct Enumerator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};
struct EnumType : cursiv::Extends<TypeSpec> {
  boolean isClass;  // `enum class`/`enum struct` (scoped)
  Name* name;
  Specifiers* underlying;  // the `: type` base, or null
  boolean defined;
  vector<Enumerator*> enumerators;
};

// -- Declarators ----

// One `*` or pointer-to-member `T::*`, or a reference `&`/`&&`: `kind` holds
// the written token(s), `memberOf` is set only for a pointer-to-member.
struct Pointer : cursiv::Extends<cursiv::AstNode> {
  string_view kind;  // "*", "&" or "&&"
  cursiv::AstNode* memberOf;  // Name* or a decltype TypeSpec*, pointer-to-member only
  vector<string_view> qualifiers;  // const/volatile, meaningful for "*" only
};

struct Suffix : cursiv::Extends<cursiv::AstNode> {};
struct ArraySuffix : cursiv::Extends<Suffix> {
  Expr* size;  // null: `[]`
};
struct FunctionSuffix : cursiv::Extends<Suffix> {
  vector<Param*> params;
  boolean variadic;
  vector<string_view> cvQualifiers;
  string_view refQualifier;  // "&", "&&" or empty
  ExceptionSpec* exceptionSpec;
};

// `throw(T...)`/`throw()`, or `noexcept`/`noexcept(expr)`.
struct ExceptionSpec : cursiv::Extends<cursiv::AstNode> {
  boolean dynamic;
  vector<TypeId*> types;
  boolean isNoexcept;
  Expr* noexceptExpr;
};

// Concrete and abstract in one class, like the c port: an empty `name`
// (rather than a null one) marks the abstract form. `nested` is the
// parenthesised sub-declarator, exclusive with `name`.
struct Declarator : cursiv::Extends<cursiv::AstNode> {
  vector<Pointer*> pointers;
  boolean pack;  // a leading `...`
  Name* name;
  Declarator* nested;
  vector<Suffix*> suffixes;
  TypeId* trailingReturnType;  // set only for the trailing-return-type form
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  Specifiers* specifiers;
  Declarator* declarator;  // may be null: an unnamed/abstract parameter
  Expr* defaultValue;
};

// The specifier list of a declaration or a type-id: `qualifiers` holds
// storage-class, cv, function-specifiers, `friend`/`typedef`/`constexpr` as
// written (order carries nothing); `types` holds the type specifiers in the
// order written -- a run of builtin keywords is one, `unsigned const long` is
// two --, empty when the list holds only qualifiers.
struct Specifiers : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> qualifiers;
  vector<TypeSpec*> types;
};

// A type as written in an expression or a cast: the operand of `sizeof`,
// `alignof`, a named cast, `new`, or a template argument.
struct TypeId : cursiv::Extends<cursiv::AstNode> {
  boolean packExpansion;  // ellipsis on a type argument or exception type
  Specifiers* specifiers;
  Declarator* declarator;  // abstract; null when bare
};

// -- Initializers and declarations ----

struct InitDeclarator : cursiv::Extends<cursiv::AstNode> {
  Declarator* declarator;
  Expr* init;               // `= expr` or a bare braced-init-list
  vector<Expr*> directInit;  // the `(args)` direct-init form
};

struct Declaration : cursiv::Extends<Item> {
  Specifiers* specifiers;  // may be null: attributes-only or type-omitted forms
  vector<InitDeclarator*> declarators;
};

// One member-declarator: a normal member (declarator, optional bit-width,
// optional virtual-specifiers, optional initializer) or a pure bit-field
// (`: width` with no declarator). `init` also carries a pure-specifier's
// `= 0`.
struct MemberDeclarator : cursiv::Extends<cursiv::AstNode> {
  Declarator* declarator;  // null: a pure bit-field
  Expr* bitWidth;           // null: not a bit-field
  vector<string_view> virtualSpecifiers;
  boolean isPure;  // `= 0` on a function declarator -- gated structurally,
                   // see FunctionShapedDeclarator in CppGrammar.hpp
  Expr* init;  // may be a BracedInitList, distinguishable by its type tag
};
struct MemberDecl : cursiv::Extends<Item> {
  Specifiers* specifiers;
  vector<MemberDeclarator*> declarators;
};

struct StaticAssertDecl : cursiv::Extends<Item> {
  Expr* cond;
  string_view message;
};
struct EmptyDecl : cursiv::Extends<Item> {};
struct AttributeDecl : cursiv::Extends<Item> {};
struct AsmDecl : cursiv::Extends<Item> {
  string_view code;
};
struct NamespaceDecl : cursiv::Extends<Item> {
  boolean isInline;
  string_view name;  // empty: an anonymous namespace
  vector<Item*> items;
};
struct NamespaceAliasDecl : cursiv::Extends<Item> {
  string_view name;
  cursiv::AstNode* target;  // a (qualified) namespace Name
};
struct UsingDecl : cursiv::Extends<Item> {
  boolean typenameKw;
  Name* name;
};
struct UsingDirective : cursiv::Extends<Item> {
  cursiv::AstNode* name;  // a (qualified) namespace Name
};
struct AliasDecl : cursiv::Extends<Item> {
  string_view name;
  TypeId* type;
};
struct LinkageSpec : cursiv::Extends<Item> {
  string_view lang;
  vector<Item*> items;
};

// -- Templates ----

struct TemplateParam : cursiv::Extends<cursiv::AstNode> {};
// `class T`, `typename T = int`, `class... Ts`, or `template<...> class T`.
struct TypeTemplateParam : cursiv::Extends<TemplateParam> {
  boolean pack;
  string_view name;
  TypeId* defaultType;
  vector<TemplateParam*> templateParams;  // the `template<...>` form; empty otherwise
};
// A non-type template parameter: an ordinary parameter declaration.
struct ParamTemplateParam : cursiv::Extends<TemplateParam> {
  Param* param;
};
struct TemplateDecl : cursiv::Extends<Item> {
  vector<TemplateParam*> params;
  Item* body;
};
struct ExplicitInstantiation : cursiv::Extends<Item> {
  boolean isExtern;
  Item* decl;
};
struct ExplicitSpecialization : cursiv::Extends<Item> {
  Item* decl;
};

// -- Statements ----

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct CompoundStmt : cursiv::Extends<Stmt> {
  vector<Item*> items;
};

// `if (T x = v)`, `while (T x = v)`, `switch (T x = v)`: a condition that
// declares. Plain conditions hold their Expr directly rather than through
// this node.
// `init` may itself be a BracedInitList, distinguishable by its type tag.
struct Condition : cursiv::Extends<cursiv::AstNode> {
  Specifiers* specifiers;
  Declarator* declarator;
  Expr* init;
};

// `body`/`thenBranch`/`elseBranch` etc. below are `Item*`, not `Stmt*`: a
// declaration is as valid there as a statement (`if (c) int x;`).
struct IfStmt : cursiv::Extends<Stmt> {
  cursiv::AstNode* cond;  // Expr* or Condition*
  Item* thenBranch;
  Item* elseBranch;
};
struct SwitchStmt : cursiv::Extends<Stmt> {
  cursiv::AstNode* cond;
  Item* body;
};
struct WhileStmt : cursiv::Extends<Stmt> {
  cursiv::AstNode* cond;
  Item* body;
};
struct DoStmt : cursiv::Extends<Stmt> {
  Item* body;
  Expr* cond;
};
struct ForStmt : cursiv::Extends<Stmt> {
  Item* init;  // an ExprStmt or a Declaration
  Expr* cond;
  Expr* step;
  Item* body;
};
struct ForRangeStmt : cursiv::Extends<Stmt> {
  Specifiers* specifiers;
  Declarator* declarator;
  Expr* range;
  Item* body;
};
struct BreakStmt : cursiv::Extends<Stmt> {};
struct ContinueStmt : cursiv::Extends<Stmt> {};
// `value` may itself be a BracedInitList (`return {1, 2};`), distinguishable
// by its type tag -- no separate flag needed.
struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};
struct GotoStmt : cursiv::Extends<Stmt> {
  string_view target;
};
struct Label : cursiv::Extends<Stmt> {
  string_view name;
  Item* body;
};
struct CaseLabel : cursiv::Extends<Stmt> {
  Expr* value;
  Item* body;
};
struct DefaultLabel : cursiv::Extends<Stmt> {
  Item* body;
};

// One member-initializer of a constructor's initializer list:
// `Base(args)`/`member{args}`.
struct MemInit : cursiv::Extends<cursiv::AstNode> {
  boolean pack;
  cursiv::AstNode* target;  // Name* or a decltype TypeSpec*
  vector<Expr*> args;
  boolean braced;
};
struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  boolean catchAll;  // `catch (...)`
  Specifiers* specifiers;
  Declarator* declarator;
  CompoundStmt* body;
};
struct TryStmt : cursiv::Extends<Stmt> {
  vector<MemInit*> ctorInit;  // a function-try-block's own init list; empty for a plain try
  CompoundStmt* body;
  vector<CatchClause*> handlers;
};

struct FunctionDef : cursiv::Extends<Item> {
  Specifiers* specifiers;
  Declarator* declarator;
  vector<string_view> virtualSpecifiers;
  vector<MemInit*> ctorInit;
  CompoundStmt* body;
  boolean isDefault;
  boolean isDelete;
  vector<CatchClause*> handlers;  // set for a function-try-block body
};

struct TranslationUnit : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace cpp::ast
