// The lpc language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace lpc::ast {

// Anything a block or the program may hold: a declaration or a statement.
struct Item : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<Item> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// A declared type: modifiers (nomask, private, protected, public, static,
// varargs), an optional builtin keyword or `class Name`. Either may be
// absent -- an untyped declaration defaults to `mixed`.
struct SimpleType;
struct TypeSpec : cursiv::Extends<cursiv::AstNode> {
  boolean arrayKeyword;
  SimpleType* typeArgument;
  vector<string_view> modifiers;
  string_view baseType;
  string_view className;
};

// A type as written for a cast, a parameter or a single typed declarator:
// the star sits here, not on the declared name, because one `basic_type`
// may be shared by several names each choosing their own array-ness.
struct SimpleType : cursiv::Extends<cursiv::AstNode> {
  boolean arrayKeyword;
  boolean byReference;
  SimpleType* typeArgument;
  string_view baseType;
  string_view className;
  boolean isArray;
};

struct VarName : cursiv::Extends<cursiv::AstNode> {
  boolean isArray;
  string_view name;
  Expr* init;
  string_view initOp;
};

struct VarDecl : cursiv::Extends<Item> {
  TypeSpec* type;
  vector<VarName*> names;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  SimpleType* type;  // null: an untyped (mixed) parameter
  string_view name;  // empty: an unnamed parameter
  Expr* defaultValue;
};

// Shared by a declared function and an inline function value: a parameter
// list, trailing `...` with no comma before it.
struct ParamList : cursiv::Extends<cursiv::AstNode> {
  vector<Param*> params;
  boolean variadic;
};

struct CompoundStmt;

struct FuncDecl : cursiv::Extends<Item> {
  TypeSpec* returnType;
  boolean returnsArray;
  string_view name;
  ParamList* params;
  CompoundStmt* body;  // null: a prototype (`;` in place of a block)
};

struct ModifierChange : cursiv::Extends<Item> {
  vector<string_view> modifiers;
};

struct ClassDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  string_view name;
  vector<VarDecl*> members;
};

struct InheritDecl : cursiv::Extends<Item> {
  vector<string_view> modifiers;
  Expr* path;  // a string literal, or a parenthesised/`+`-joined run of them
};

// -- Expressions --

struct IdentExpr : cursiv::Extends<Expr> {
  string_view name;
};

struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

struct CharLit : cursiv::Extends<Expr> {
  string_view text;
};

// Adjacent string literals concatenate; the pieces stay separate so a
// formatter can print them back as written.
struct StringLit : cursiv::Extends<Expr> {
  vector<string_view> pieces;
};

struct TemplatePart : cursiv::Extends<cursiv::AstNode> {
  string_view text;
  Expr* expression;
};
struct TemplateString : cursiv::Extends<Expr> { vector<TemplatePart*> parts; };
struct StringConcat : cursiv::Extends<Expr> { Expr* left; Expr* right; };

struct ParamRefExpr : cursiv::Extends<Expr> {  // `$1`, `$2`...
  string_view text;
};

struct EvalExpr : cursiv::Extends<Expr> {  // `$(expr)`
  Expr* index;
};

// The callee of a call: a plain name, `::name` (root scope), `Type::name`
// or `id::name` (an inherited/qualified call), or `efun::name`.
struct NameExpr : cursiv::Extends<Expr> {
  boolean hasQualifier;
  string_view qualifier;
  string_view name;
};

struct Arg : cursiv::Extends<cursiv::AstNode> {
  Expr* value;
  boolean spread;  // a trailing `...`
};

// Shared wherever a parenthesised (or braced) comma-separated run of
// arguments occurs: a call, a method call, an array literal.
struct ArgList : cursiv::Extends<cursiv::AstNode> {
  vector<Arg*> items;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee;
  ArgList* args;
};

struct MemberExpr : cursiv::Extends<Expr> {
  boolean optional;
  boolean dot;  // `object.field`; false for `object->field`
  Expr* object;
  string_view field;
};

struct MethodCallExpr : cursiv::Extends<Expr> {  // `object->method(args)`
  Expr* object;                                  // null: no receiver written
  string_view method;
  ArgList* args;
};

struct ClassInit : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value;
};

struct NewClassExpr : cursiv::Extends<Expr> {  // `new(class Name, f: v, ...)`
  string_view className;
  vector<ClassInit*> inits;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* array;
  boolean optional;
  boolean fromEnd;  // a leading `<` on the index
  Expr* index;
};

// Both endpoints may use `<` for a reverse index from the end.
struct SliceExpr : cursiv::Extends<Expr> {
  Expr* array;
  boolean lowFromEnd;
  Expr* low;
  boolean highFromEnd;
  boolean exclusive; // legacy alias for highFromEnd, not an exclusive bound
  // Source `<` marks a reverse index from the end.
  Expr* high;         // null: open-ended (`a[lo..]`)
};

struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct PostfixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand;
};

struct CastExpr : cursiv::Extends<Expr> {
  SimpleType* type;
  Expr* operand;
};

// Every binary level (arithmetic, bitwise, relational, logical) and the
// comma operator: one shape, `op` carries which.
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

struct ClosureExpr : cursiv::Extends<Expr> {
  Expr* callee;
  ArgList* arguments;  // `(: efun::name, bound, args :)`
  Expr* body;
};

struct AssocPair : cursiv::Extends<cursiv::AstNode> {
  Expr* key;
  Expr* value;
};

struct MappingExpr : cursiv::Extends<Expr> {  // `([ k: v, ... ])`
  vector<AssocPair*> pairs;
};

struct ArrayExpr : cursiv::Extends<Expr> {  // `({ a, b, ... })`
  ArgList* items;
};

struct FuncLitExpr : cursiv::Extends<Expr> {  // an inline function value
  string_view returnType;  // one builtin keyword -- no modifiers, no class
  ParamList* params;
  CompoundStmt* body;
};

struct CatchExpr : cursiv::Extends<Expr> {
  std::variant<CompoundStmt*, Expr*> body;
};

struct AcatchExpr : cursiv::Extends<Expr> { std::variant<CompoundStmt*, Expr*> body; };

struct TimeExpr : cursiv::Extends<Expr> {
  std::variant<CompoundStmt*, Expr*> body;
};

struct SscanfExpr : cursiv::Extends<Expr> {
  Expr* input;
  Expr* format;
  vector<Expr*> lvalues;
};

struct ParseCommandExpr : cursiv::Extends<Expr> {
  Expr* command;
  Expr* rule1;
  Expr* rule2;
  vector<Expr*> lvalues;
};

// -- Statements --

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct CompoundStmt : cursiv::Extends<Stmt> {
  vector<Item*> items;
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

struct DoWhileStmt : cursiv::Extends<Stmt> {
  Stmt* body;
  Expr* cond;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Expr* initExpr;        // the plain-expression form of the init clause
  Param* initVar;        // ...or a typed declarator
  string_view initVarOp;
  Expr* initVarValue;    // its mandatory initialiser, paired with initVar
  Expr* cond;
  Expr* step;
  Stmt* body;
};

struct ForeachStmt : cursiv::Extends<Stmt> {
  vector<Param*> vars;  // one (`k in m`) or two (`k, v in m`)
  Expr* iterable;
  Stmt* body;
};

struct CaseStmt : cursiv::Extends<Stmt> {
  boolean range;
  Expr* low;
  Expr* high;  // null for an open high bound
};

struct DefaultStmt : cursiv::Extends<Stmt> {};

// A switch body is a flat run of case/default labels and statements, exactly
// as written: everything after a label until the next one is that case's.
struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value;
  vector<VarDecl*> localDecls;
  vector<Stmt*> items;
};

struct BreakStmt : cursiv::Extends<Stmt> {};
struct ContinueStmt : cursiv::Extends<Stmt> {};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Item*> items;
};

}  // namespace lpc::ast
