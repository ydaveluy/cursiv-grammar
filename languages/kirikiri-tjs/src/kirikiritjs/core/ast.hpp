// The kirikiritjs language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace kirikiritjs::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// -- Expressions ----

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct ThisExpr : cursiv::Extends<Expr> {};
struct SuperExpr : cursiv::Extends<Expr> {};
struct NullLit : cursiv::Extends<Expr> {};
struct VoidLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// The spelling, not the value: separators and a leading sign belong to a
// later pass.
struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

// `<% ... %>`, kept raw with its delimiters.
struct OctetLit : cursiv::Extends<Expr> {
  string_view text;
};

struct RegexLit : cursiv::Extends<Expr> {
  string_view text; // slashes and flags included
};

// One or more adjacent string/template pieces, concatenated by the language;
// each part keeps its own quotes and escapes raw.
struct StringLit : cursiv::Extends<Expr> {
  vector<string_view> parts;
};

// `'(' expressions ')'`: a list even for the common one-expression case, so
// the node shape stays uniform.
struct ParenExpr : cursiv::Extends<Expr> {
  vector<Expr *> expressions;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  Ident *name; // null for a bare `*` rest parameter
  Expr *defaultValue;
  boolean rest;
};

struct Block;

struct FunctionExpr : cursiv::Extends<Expr> {
  vector<Param *> params;
  Block *body;
};

struct ArraySlot : cursiv::Extends<cursiv::AstNode> {
  Expr *value;
};
struct ArrayLit : cursiv::Extends<Expr> {
  string_view varModifier; // "var" | "const" | empty
  vector<ArraySlot *> elements;
};

struct Prop : cursiv::Extends<cursiv::AstNode> {
  Expr *key;
  Expr *value;
};

struct ObjectLit : cursiv::Extends<Expr> {
  string_view varModifier;
  vector<Prop *> properties;
};

// `expression '*'?` or a bare `*`: `value` is null for the bare form.
struct Arg : cursiv::Extends<cursiv::AstNode> {
  Expr *value;
  boolean star;
};

// `object` null for the implicit ("with"-context) form. `computed` selects
// `indices` (a bracketed, comma-separated list) over `property` (a dotted
// name, itself an Ident or a reserved word spelled as one).
struct MemberExpr : cursiv::Extends<Expr> {
  Expr *object;
  Expr *property;
  vector<Expr *> indices;
  boolean computed;
};

// `spreadArgs`: called as `f(...)`, forwarding the enclosing function's own
// arguments -- `args` is then empty.
struct NewExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<Arg *> args;
  boolean spreadArgs;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<Arg *> args;
  boolean spreadArgs;
};

// `#x`: the byte/character code of a one-character string, or of a variant
// read as a number; real scripts across this corpus write
// it consistently as a tight prefix operator -- ported as one.
struct CharCodeExpr : cursiv::Extends<Expr> {
  Expr *operand;
};

struct ReferenceExpr : cursiv::Extends<Expr> { // `&x`
  Expr *operand;
};
struct PointerExpr : cursiv::Extends<Expr> { // `*x`
  Expr *operand;
};
struct EvalExpr : cursiv::Extends<Expr> { // postfix `x!`
  Expr *operand;
};
// Prefix (`isvalid x`) and postfix (`x isvalid`): TJS2 has two positions
// for one check, so one class covers both.
struct IsValidExpr : cursiv::Extends<Expr> {
  Expr *operand;
};
struct DeleteExpr : cursiv::Extends<Expr> {
  Expr *operand;
};
struct TypeofExpr : cursiv::Extends<Expr> {
  Expr *operand;
};
struct InvalidateExpr : cursiv::Extends<Expr> {
  Expr *operand;
};

struct PrefixUpdateExpr : cursiv::Extends<Expr> { // `++x`, `--x`
  string_view op;
  Expr *operand;
};
struct PostfixUpdateExpr : cursiv::Extends<Expr> { // `x++`, `x--`
  string_view op;
  Expr *operand;
};
struct UnaryExpr : cursiv::Extends<Expr> { // `+x -x ~x !x`
  string_view op;
  Expr *operand;
};

// Every plain binary operator: `<->`, `incontextof`, `instanceof`, `in`, the
// arithmetic/shift/relational/equality/bitwise/logical ladder.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  string_view op;
  Expr *right;
};

struct TernaryExpr : cursiv::Extends<Expr> {
  Expr *test;
  Expr *consequent;
  Expr *alternate;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr *target;
  string_view op;
  Expr *value;
};

struct TypeConvertExpr : cursiv::Extends<Expr> {
  string_view targetType; // "int" | "real" | "string"
  Expr *operand;
};

// The postfix statement-modifier form: `stmt-as-expr if cond, cond2`.
struct IfExpr : cursiv::Extends<Expr> {
  Expr *body;
  vector<Expr *> tests;
};

// -- Statements ----

struct Block : cursiv::Extends<Stmt> {
  vector<Stmt *> body;
};

struct Declarator : cursiv::Extends<cursiv::AstNode> {
  Ident *id;
  Expr *init;
};

struct VarDecl : cursiv::Extends<Stmt> {
  string_view kind; // "var" | "const"
  vector<Declarator *> declarations;
};

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct ExprStmt : cursiv::Extends<Stmt> {
  vector<Expr *> expressions;
};

struct IfStmt : cursiv::Extends<Stmt> {
  vector<Expr *> test;
  Stmt *consequent;
  Stmt *alternate;
};

struct DoWhileStmt : cursiv::Extends<Stmt> {
  Stmt *body;
  vector<Expr *> test;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  vector<Expr *> test;
  Stmt *body;
};

// One class for both the classic and the var-declaring `for`: `initDecl` and
// `initExprs` are never both non-empty.
struct ForStmt : cursiv::Extends<Stmt> {
  VarDecl *initDecl;
  vector<Expr *> initExprs;
  vector<Expr *> test;
  vector<Expr *> update;
  Stmt *body;
};

struct ContinueStmt : cursiv::Extends<Stmt> {};
struct BreakStmt : cursiv::Extends<Stmt> {};

struct ReturnStmt : cursiv::Extends<Stmt> {
  vector<Expr *> argument;
};

struct WithStmt : cursiv::Extends<Stmt> {
  vector<Expr *> object;
  Stmt *body;
};

struct SwitchCase : cursiv::Extends<cursiv::AstNode> {
  vector<Expr *> test; // empty: `default`
  vector<Stmt *> consequent;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  vector<Expr *> discriminant;
  vector<SwitchCase *> cases;
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  vector<Expr *> argument;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  Ident *param; // null: catch with no binding
  Block *body;
};

// `catch` is mandatory; TJS has no `finally`.
struct TryStmt : cursiv::Extends<Stmt> {
  Block *block;
  CatchClause *handler;
};

struct DebuggerStmt : cursiv::Extends<Stmt> {};

struct FunctionDecl : cursiv::Extends<Stmt> {
  string_view name;
  vector<Param *> params;
  Block *body;
};

struct Accessor : cursiv::Extends<cursiv::AstNode> {
  boolean isGetter;
  boolean isSetter;
  Param *param; // setter only
  Block *body;
};

struct PropertyDecl : cursiv::Extends<Stmt> {
  string_view name;
  vector<Accessor *> accessors;
};

// The body is a plain block: TJS classes hold ordinary statements, not a
// dedicated member grammar. Multiple base classes: TJS allows them.
struct ClassDecl : cursiv::Extends<Stmt> {
  string_view name;
  vector<Expr *> superClasses;
  Block *body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt *> body;
};

} // namespace kirikiritjs::ast
