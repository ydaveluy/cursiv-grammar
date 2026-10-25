// Unreal AngelScript: its AST. Names, nested-name-specifiers and
// pseudo-destructor spellings are kept as flat text rather than a structured
// path -- a formatter reads the spelling back, nothing here resolves scope.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace unrealangelscript::ast {

// Anything a declaration list may hold: an import, a class member, a
// statement, a function definition. The grammar, not this type, keeps a
// function definition out of a place that wants only a statement.
struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<Item> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Specifiers;
struct UType;
struct UPropertyAnno;
struct UFunctionAnno;
struct CompoundStmt;
struct Declarator;

// -- Literals and names ----------------------------------------------------

// The spelling, not the value: kept as written since a converter would need
// to know the target's width. One class per lexical kind, so a type test
// replaces re-lexing.
struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

// Kept verbatim, prefix (`n"`, `f"`, `"""`) included.
struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// An integer/float/char/string literal with a user-defined suffix, kept
// verbatim: the suffix names a conversion this parser does not resolve.
struct UserLit : cursiv::Extends<Expr> {
  string_view text;
};

struct NullLit : cursiv::Extends<Expr> {};

// `literal+`: adjacent literals (concatenated string pieces, most often).
struct LiteralSeq : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct ThisExpr : cursiv::Extends<Expr> {};

struct ParenExpr : cursiv::Extends<Expr> {
  Expr* inner;
};

// A name: a plain identifier, an operator-function-id (`operator+`), a
// literal-operator-id, a destructor spelling (`~T`), any of them optionally
// qualified by a nested-name-specifier -- captured whole, as written.
struct IdExpr : cursiv::Extends<Expr> {
  string_view name;
};

// -- Types -------------------------------------------------------------

struct TypeSpec : cursiv::Extends<cursiv::AstNode> {};

// One element of a decl-specifier-seq / type-specifier-seq.
struct Specifiers : cursiv::Extends<cursiv::AstNode> {
  vector<TypeSpec*> parts;
};

struct BuiltinTypeSpec : cursiv::Extends<TypeSpec> {
  string_view keyword;  // int, float, bool, void, auto...
};

// `const`, `&` (a reference marker) and `out` (an out-parameter marker), and
// `virtual` as a function-specifier -- all bare qualifier words.
struct QualifierSpec : cursiv::Extends<TypeSpec> {
  string_view keyword;
};

// `nestedNameSpecifier? theTypeName`: a possibly-qualified type name.
struct NamedTypeSpec : cursiv::Extends<TypeSpec> {
  string_view name;
};

// `Identifier < simpleTypeSpecifierList >` -- each argument is itself a
// decl-specifier-seq, so `TMap<FString, TArray<int32>>` nests naturally.
struct GenericTypeSpec : cursiv::Extends<TypeSpec> {
  string_view name;
  vector<Specifiers*> args;
};

struct DecltypeSpec : cursiv::Extends<TypeSpec> {
  Expr* expr;     // null when `isAuto`
  boolean isAuto;
};

// `class`/`struct`/`enum` naming a tag without defining it:
// `struct S`, `enum NS::Color`.
struct ElaboratedTypeSpec : cursiv::Extends<TypeSpec> {
  string_view keyword;
  string_view name;
};

// -- Classes and enums (also type-specifiers: a definition may appear
// wherever a type does) ----------------------------------------------------

struct BaseSpecifier : cursiv::Extends<cursiv::AstNode> {
  boolean isVirtual;
  string_view access;  // empty when absent
  string_view name;    // className text; empty when `declType` is set
  DecltypeSpec* declType;
};

struct ClassDef : cursiv::Extends<TypeSpec> {
  UType* utype;
  string_view keyword;  // "class" | "struct"
  string_view name;     // empty for an inline anonymous use, if any
  boolean isFinal;
  vector<BaseSpecifier*> bases;
  vector<Item*> members;
};

struct Enumerator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct EnumDef : cursiv::Extends<TypeSpec> {
  string_view name;
  Specifiers* base;  // enumbase, or null
  vector<Enumerator*> enumerators;
};

// `enum Color;` -- a forward reference to a tag, no member list.
struct EnumForwardDecl : cursiv::Extends<Item> {
  string_view name;
  Specifiers* base;
};

// -- UCLASS/USTRUCT/UPROPERTY/UFUNCTION annotations -------------------------

struct Annotation : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct UType : cursiv::Extends<cursiv::AstNode> {
  string_view keyword;  // "UCLASS" | "USTRUCT"
  vector<Annotation*> annotations;
};

struct UPropertyAnno : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
};

struct UFunctionAnno : cursiv::Extends<cursiv::AstNode> {
  vector<Annotation*> annotations;
};

// -- Initializers ------------------------------------------------------

struct Initializer : cursiv::Extends<cursiv::AstNode> {};

struct ExprInit : cursiv::Extends<Initializer> {
  Expr* value;
};

struct ListInit : cursiv::Extends<Initializer> {
  vector<Initializer*> items;
};

// -- Declarators ---------------------------------------------------------

struct Param : cursiv::Extends<cursiv::AstNode> {
  Specifiers* specifiers;
  string_view name;
  Initializer* defaultValue;
};

// `( parameter-list? ) const? refqualifier?`
struct Params : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> params;
  boolean isConst;
  string_view refQualifier;  // "&" | "&&" | empty
};

// A function declarator: `Name(args) const &`, always ending in a mandatory
// parameter list, so this class covers only the function-shaped case; a
// plain data member is a bare Identifier (`MemberDeclarator::name`), not a
// Declarator.
struct Declarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Params* params;
};

struct InitDeclarator : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Initializer* init;           // braceOrEqualInitializer
  vector<Initializer*> callArgs;  // direct-init: `Name(args)`
};

// A member: either a function-shaped declarator, or a bare name, optionally
// with a bit-field width.
struct MemberDeclarator : cursiv::Extends<cursiv::AstNode> {
  Declarator* declarator;
  vector<string_view> postSpecifiers;
  Initializer* init;
  string_view name;
  Expr* bitWidth;
};

// -- Expressions -----------------------------------------------------------

struct LambdaCapture : cursiv::Extends<cursiv::AstNode> {
  boolean byRef;
  boolean isThis;
  string_view name;
  Initializer* init;  // set only for an init-capture
};

struct LambdaExpr : cursiv::Extends<Expr> {
  string_view captureDefault;  // "&" | "=" | empty
  vector<LambdaCapture*> captures;
  vector<Param*> params;
  CompoundStmt* body;
};

struct AssertExpr : cursiv::Extends<Expr> {
  string_view kind;  // "ensure" | "ensureAlways" | "check"
  vector<Initializer*> args;
};

// `Type(args)` / `Type{list}` -- construction of a builtin or generic type.
// A plain-name target (`Foo(1, 2)`) is a CallExpr instead: the two would
// otherwise parse the same tokens two ways.
struct ConstructExpr : cursiv::Extends<Expr> {
  TypeSpec* type;
  vector<Initializer*> args;
  ListInit* list;
};

// `Cast<T>(expr)`.
struct CastExpr : cursiv::Extends<Expr> {
  Specifiers* type;
  Expr* operand;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  vector<Initializer*> args;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;      // set for the plain-expression form
  ListInit* indexList;  // set for the rare bracedInitList form
};

// `s.f`, `p->f` (arrow spelled the same as dot here -- the grammar has no
// separate Arrow token) and `p.~T()`: `field` carries whatever followed the
// dot, verbatim.
struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

struct PostfixOpExpr : cursiv::Extends<Expr> {
  string_view op;  // "++" | "--"
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

struct CondExpr : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* thenValue;
  Expr* elseValue;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target;
  string_view op;
  Initializer* value;
};

// -- Statements --------------------------------------------------------

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct CompoundStmt : cursiv::Extends<Stmt> {
  vector<Item*> items;
};

// A declaration used as an `if`/`while`/`switch` condition:
// `if (int x = f())`. Extends Expr so `IfStmt::cond` et al. stay one field.
struct CondDecl : cursiv::Extends<Expr> {
  Specifiers* specifiers;
  Declarator* declarator;
  Initializer* init;  // set for the `= initializerClause` branch
  ListInit* list;     // set for the bracedInitList branch
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* thenBranch;
  Stmt* elseBranch;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  Stmt* body;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct DoStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

struct ForRangeDecl : cursiv::Extends<cursiv::AstNode> {
  Specifiers* specifiers;
  string_view name;
};

// Both the classic C-style clause and the range-based form, in one class:
// `rangeDecl` is set instead of `init`/`cond`/`step` for the range form.
struct ForStmt : cursiv::Extends<Stmt> {
  Item* init;
  Expr* cond;
  Expr* step;
  ForRangeDecl* rangeDecl;
  Initializer* rangeInit;
  Stmt* body;
};

struct Label : cursiv::Extends<Stmt> {
  string_view name;
  Stmt* body;
};

struct CaseLabel : cursiv::Extends<Stmt> {
  Expr* value;
  Stmt* body;
};

struct DefaultLabel : cursiv::Extends<Stmt> {
  Stmt* body;
};

struct BreakStmt : cursiv::Extends<Stmt> {};
struct ContinueStmt : cursiv::Extends<Stmt> {};

struct GotoStmt : cursiv::Extends<Stmt> {
  string_view target;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
  ListInit* list;
};

// -- Declarations --------------------------------------------------------

struct EmptyDecl : cursiv::Extends<Item> {};

// `import A.B.C;`
struct ModuleImport : cursiv::Extends<Item> {
  vector<string_view> path;
};

// `import <declarator> from "module";`
struct ImportFunction : cursiv::Extends<Item> {
  Specifiers* specifiers;
  Declarator* declarator;
  vector<string_view> postSpecifiers;
  string_view from;
};

struct NamespaceDef : cursiv::Extends<Item> {
  string_view name;
  vector<Item*> items;
};

struct NamespaceAlias : cursiv::Extends<Item> {
  string_view name;
  string_view target;
};

struct AliasDecl : cursiv::Extends<Item> {
  string_view name;
  Specifiers* type;
};

// `declSpecifierSeq? (initDeclaratorList | assignmentExpression)? ;` -- a
// variable declaration, a bare type definition (class/struct/enum), or a
// bare expression statement at declaration scope.
// Extends Stmt, not Item directly: a restricted form of this same class is
// also reachable through Statement (DeclStmtRule), where every alternative
// must share the Stmt family for `Label::body`-shaped members to type-check.
struct VarDecl : cursiv::Extends<Stmt> {
  Specifiers* specifiers;
  vector<InitDeclarator*> declarators;
  Expr* expr;
};

struct FunctionDef : cursiv::Extends<Item> {
  UFunctionAnno* ufunction;
  string_view access;
  boolean isMixin;
  Specifiers* specifiers;
  Declarator* declarator;
  vector<string_view> postSpecifiers;
  CompoundStmt* body;
  boolean isDefaulted;  // `= default;`
};

struct AccessLabel : cursiv::Extends<Item> {
  string_view access;
};

struct PropertyDef : cursiv::Extends<Item> {
  UPropertyAnno* uproperty;
  string_view access;
  boolean isDefault;
  Specifiers* specifiers;
  vector<MemberDeclarator*> declarators;
  Expr* expr;
};

struct Script : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace unrealangelscript::ast
