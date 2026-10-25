// The V language: its AST. Types are syntactic only -- V's own type checker
// is out of scope, so NamedType/QualifiedType stay unresolved names.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace v::ast {

// Base for anything a list of items may hold: a top-level declaration or a
// statement. SourceFile::decls and Block::stmts share it.
struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Stmt : cursiv::Extends<Item> {};

// An expression. Empty for the same reason as c's: every alternative of the
// primary choice fits any operand slot the family check is against.
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct Type : cursiv::Extends<cursiv::AstNode> {};

struct Block;
struct MatchArm : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> patterns;
  boolean isElse;
  Block* body;
};
struct MatchExpr : cursiv::Extends<Expr> {
  Expr* subject;
  vector<MatchArm*> arms;
};
struct EnumValue : cursiv::Extends<Expr> {
  string_view name;
};
struct OrExpr : cursiv::Extends<Expr> {
  Expr* operand;
  Block* handler;
};
struct EnumMember : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};
struct EnumDecl : cursiv::Extends<Item> {
  boolean isPub;
  string_view name;
  vector<EnumMember*> members;
};

// -- Expressions ----

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

// A rune literal and a string literal are both single-quoted and match the
// same bytes at length one, so one class serves both forms.
struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

struct ArrayLit : cursiv::Extends<Expr> {
  vector<Expr*> elements;
};

// `key: value`, inside a `{...}` literal. `key` is null for a positional
// element. Both may themselves be a nested `LiteralValue` (bare `{...}`),
// which is why they hold `Expr*` and not something narrower.
struct KeyedElement : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
};

// `{...}`, untyped -- an element or a key of an enclosing literal, or the
// value half of a `CompositeLit` once a type precedes it.
struct LiteralValue : cursiv::Extends<Expr> {
  vector<KeyedElement*> elements;
};

struct CompositeLit : cursiv::Extends<Expr> {
  Type* type;
  LiteralValue* value;
};

struct Signature;
struct Block;

struct FunctionLit : cursiv::Extends<Expr> {
  Signature* signature;
  Block* body;
};

// `T(x)`: a type conversion.
struct ConversionExpr : cursiv::Extends<Expr> {
  Type* type;
  Expr* value;
};

// `x.field`. Also stands for a package-qualified name (`pkg.Name`) and a
// method-expression (`T.Method`): all three read the same `name '.' name`
// as this postfix step, so folding them in here avoids a redundant
// derivation of the same input.
struct SelectorExpr : cursiv::Extends<Expr> {
  Expr* object;
  string_view field;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  Expr* index;
};

// `f(a, b)`, and V's `T(...)`/`make`-shaped calls that lead with a type:
// `typeArg` is set for those, alongside or instead of `args`.
struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  Type* typeArg;
  vector<Expr*> args;
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

// -- Types ----

struct NamedType : cursiv::Extends<Type> {
  boolean pointer;  // a leading `&`
  boolean mut;
  string_view name;
};

struct QualifiedType : cursiv::Extends<Type> {
  string_view pkg;
  string_view name;
};

struct ArrayType : cursiv::Extends<Type> {
  Expr* length;  // null for `[]T`
  Type* element;
};

struct FunctionType : cursiv::Extends<Type> {
  Signature* signature;
};

struct MethodSpec;

struct InterfaceType : cursiv::Extends<Type> {
  string_view name;
  vector<MethodSpec*> methods;
};

struct MapType : cursiv::Extends<Type> {
  Type* key;
  Type* element;
};

// -- Signatures ----

struct Param : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;  // empty for an unnamed parameter
  Type* type;
};

// A signature's result: either a parenthesised parameter list (several
// return values) or a single bare type.
struct Result_ : cursiv::Extends<cursiv::AstNode> {
  boolean isParams;
  vector<Param*> params;
  Type* type;
};

struct Signature : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> params;
  Result_* result;
};

// One line of an `interface { ... }` body: a method (name set) or an
// embedded interface name (embedded set, name empty).
struct MethodSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Param*> params;
  Result_* result;
  Type* embedded;
};

// -- Declarations ----

struct ModuleClause : cursiv::Extends<cursiv::AstNode> {
  string_view name;
};

struct ImportSpec : cursiv::Extends<cursiv::AstNode> {
  boolean dot;             // `import . path`
  optional<string_view> alias;
  string_view path;        // an identifier's or a string literal's raw text
};

struct ConstSpec : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct ConstDecl : cursiv::Extends<Item> {
  vector<ConstSpec*> specs;
};

struct VarDecl : cursiv::Extends<Item> {
  boolean mut;
  vector<string_view> names;
  vector<Expr*> values;
};

// A struct field, or a visibility section marker (`pub:`, `mut:`,
// `pub mut:`) that applies to the fields following it -- `isSection` tells
// the two apart, since both live in `StructDecl::fields`.
struct FieldDecl : cursiv::Extends<cursiv::AstNode> {
  boolean isSection;
  boolean pub;
  boolean mut;
  vector<string_view> names;
  Type* type;
  string_view tag;  // an optional trailing string literal
};

struct StructDecl : cursiv::Extends<Item> {
  string_view name;
  vector<FieldDecl*> fields;
};

// A plain function (`receiver` empty) or a method (`receiver` the parameter
// list V's grammar allows there) -- one class for both, matching how close
// the two productions already are.
struct FunctionDecl : cursiv::Extends<Item> {
  vector<Param*> receiver;
  string_view name;
  Signature* signature;
  Block* body;  // null for a bare signature declaration
};

// -- Statements ----

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct IncDecStmt : cursiv::Extends<Stmt> {
  Expr* operand;
  string_view op;
};

struct AssignStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;
  string_view op;
  vector<Expr*> values;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  vector<Expr*> values;
};

struct BreakStmt : cursiv::Extends<Stmt> {};

struct ContinueStmt : cursiv::Extends<Stmt> {};

struct Block : cursiv::Extends<Stmt> {
  vector<Item*> stmts;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Item* init;  // a simple statement before `;`, or null
  Expr* cond;
  Block* thenBranch;
  Item* elseBranch;  // an IfStmt, a Block, or null
};

struct CaseClause : cursiv::Extends<cursiv::AstNode> {
  vector<Expr*> values;  // empty when `isDefault`
  boolean isDefault;
  vector<Item*> stmts;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Item* init;
  Expr* tag;  // null for a tagless switch
  vector<CaseClause*> cases;
};

// `for {}`, `for cond {}`, `for init; cond; post {}` and `for x in y {}` all
// land here. `isForClause` tells the three-part C-style form (semicolons
// written, parts possibly empty) from the plain `cond`-only form; a
// non-empty `inIterable` marks the for-in form.
struct ForStmt : cursiv::Extends<Stmt> {
  boolean isForClause;
  Item* init;
  Expr* cond;
  Item* post;
  vector<Expr*> inTargets;
  string_view inIterable;
  Block* body;
};

// -- Document ----

struct SourceFile : cursiv::Extends<cursiv::AstNode> {
  ModuleClause* module;
  vector<ImportSpec*> imports;
  vector<Item*> decls;
};

}  // namespace v::ast
