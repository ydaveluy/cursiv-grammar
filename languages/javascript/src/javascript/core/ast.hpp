// The javascript language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace javascript::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// -- Expressions ---- A binding target (a declarator's name, a parameter, a
// catch binding, the left side of a for-in/for-of) is not its own grammar:
// destructuring reuses array/object literal and identifier syntax verbatim,
// so every binding target below is an `Expr*` -- an `Ident`, an `ArrayLit`,
// or an `ObjectLit` -- and whether it is really a valid pattern is left to a
// later pass.

struct Ident : cursiv::Extends<Expr> {
  string_view name;
};

struct ThisExpr : cursiv::Extends<Expr> {};
struct SuperExpr : cursiv::Extends<Expr> {};

// The literal property word is retained for source tooling.
struct NewTargetExpr : cursiv::Extends<Expr> {
  Expr *property;
};

struct ImportMetaExpr : cursiv::Extends<Expr> {
  Expr *property;
};

struct NullLit : cursiv::Extends<Expr> {};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

// The spelling, not the value: a numeric literal's meaning depends on
// separators and suffixes a parser should not resolve.
struct NumberLit : cursiv::Extends<Expr> {
  string_view text;
};

struct BigIntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expr> {
  string_view text; // quotes included, escapes raw
};

struct RegexLit : cursiv::Extends<Expr> {
  string_view text; // slashes and flags included
};

// Untagged when `tag` is null. Quasis preserve escape spelling and normalize
// physical CR/CRLF to LF, as the ECMAScript raw template value does.
// `quasis.size() == expressions.size() + 1`.
struct TemplateLit : cursiv::Extends<Expr> {
  Expr *tag;
  vector<string_view> quasis;
  vector<Expr *> expressions;
};

// `...x`: an array/argument element, never a property (an object spread
// sets `Prop::key` to null instead).
struct Spread : cursiv::Extends<Expr> {
  Expr *argument;
};

struct ArraySlot : cursiv::Extends<cursiv::AstNode> {
  Expr *value; // null is an elision
};
struct ArrayLit : cursiv::Extends<Expr> {
  vector<ArraySlot *> elements;
};

struct Prop : cursiv::Extends<cursiv::AstNode> {
  boolean computed;
  boolean isGetter;
  boolean isSetter;
  boolean isMethod;
  boolean isAsync;
  boolean isGenerator;
  // Both null for `...expr` (spread) and for the permissive `{expr}`
  // shorthand: tell them apart by whether `value` is a `Spread`.
  Expr *key; // null for a spread or shorthand property
  Expr
      *value; // FunctionExpr* for get/set/method; the argument when key is null
};

struct ObjectLit : cursiv::Extends<Expr> {
  vector<Prop *> properties;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  boolean rest;
  Expr *pattern; // Ident, ArrayLit or ObjectLit
  Expr *defaultValue;
};

struct BlockStmt;

struct FunctionExpr : cursiv::Extends<Expr> {
  boolean isAsync;
  boolean isGenerator;
  string_view name; // empty when anonymous
  vector<Param *> params;
  BlockStmt *body;
};

struct ArrowFunctionExpr : cursiv::Extends<Expr> {
  boolean isAsync;
  vector<Param *> params;
  BlockStmt *block; // exactly one of block/exprBody is set
  Expr *exprBody;
};

struct ClassMember;

struct ClassExpr : cursiv::Extends<Expr> {
  string_view name; // empty when anonymous
  Expr *superClass;
  vector<ClassMember *> members;
};

// One member, tagged rather than subclassed: a tool asks `isGetter`/
// `isSetter`/`isMethod` the way it asks `MemberExpr::computed`. A field is
// what is left once those and `staticBlock` are ruled out.
struct ClassMember : cursiv::Extends<cursiv::AstNode> {
  boolean isStatic;
  boolean isGetter;
  boolean isSetter;
  boolean isMethod;
  boolean isAsync;
  boolean isGenerator;
  boolean computed;
  Expr *key; // null for a static block
  Expr
      *value; // FunctionExpr* for a method/getter/setter; a field's initializer
  BlockStmt *staticBlock;
};

struct NewExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<Expr *> args; // `new X` and `new X()` are both an empty list
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr *callee;
  vector<Expr *> args;
  boolean optional;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr *object;
  Expr *property; // Ident* unless computed; its name keeps a leading '#'
  boolean computed;
  boolean optional;
};

// `++x`, `--x`. A separate class from the postfix form and not a flag on it:
// the two compute different values, and a directive can set a member from a
// matched token but not tag a whole alternative for free.
struct PrefixUpdateExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};

struct PostfixUpdateExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};

struct UnaryExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr *operand;
};

struct AwaitExpr : cursiv::Extends<Expr> {
  Expr *operand;
};

struct YieldExpr : cursiv::Extends<Expr> {
  boolean delegate; // yield*
  Expr *argument;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr *left;
  string_view op;
  Expr *right;
};

struct ConditionalExpr : cursiv::Extends<Expr> {
  Expr *test;
  Expr *consequent;
  Expr *alternate;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr *target;
  string_view op;
  Expr *value;
};

struct ImportCallExpr : cursiv::Extends<Expr> {
  Expr *source;
  Expr *options;
};

// -- Statements ----

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct BlockStmt : cursiv::Extends<Stmt> {
  vector<Stmt *> body;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr *expression;
};

struct Declarator : cursiv::Extends<cursiv::AstNode> {
  Expr *id; // Ident, ArrayLit or ObjectLit
  Expr *init;
};

struct VarDecl : cursiv::Extends<Stmt> {
  string_view kind; // "var" | "let" | "const"
  vector<Declarator *> declarations;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr *test;
  Stmt *consequent;
  Stmt *alternate;
};

struct DoWhileStmt : cursiv::Extends<Stmt> {
  Stmt *body;
  Expr *test;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr *test;
  Stmt *body;
};

struct ForStmt : cursiv::Extends<Stmt> {
  VarDecl *initDecl;
  Expr *initExpr;
  Expr *test;
  Expr *update;
  Stmt *body;
};

struct ForInStmt : cursiv::Extends<Stmt> {
  VarDecl *leftDecl;
  Expr *leftExpr;
  Expr *right;
  Stmt *body;
};

struct ForOfStmt : cursiv::Extends<Stmt> {
  boolean isAwait;
  VarDecl *leftDecl;
  Expr *leftExpr;
  Expr *right;
  Stmt *body;
};

struct ContinueStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct BreakStmt : cursiv::Extends<Stmt> {
  string_view label;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr *argument;
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr *argument;
};

struct LabeledStmt : cursiv::Extends<Stmt> {
  string_view label;
  Stmt *body;
};

struct WithStmt : cursiv::Extends<Stmt> {
  Expr *object;
  Stmt *body;
};

struct SwitchCase : cursiv::Extends<cursiv::AstNode> {
  Expr *test; // null for `default`
  vector<Stmt *> consequent;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr *discriminant;
  vector<SwitchCase *> cases;
};

struct CatchClause : cursiv::Extends<cursiv::AstNode> {
  Expr *param; // null: catch with no binding
  BlockStmt *body;
};

struct TryStmt : cursiv::Extends<Stmt> {
  BlockStmt *block;
  CatchClause *handler;
  BlockStmt *finalizer;
};

struct DebuggerStmt : cursiv::Extends<Stmt> {};

struct FunctionDecl : cursiv::Extends<Stmt> {
  boolean isAsync;
  boolean isGenerator;
  string_view name;
  vector<Param *> params;
  BlockStmt *body;
};

struct ClassDecl : cursiv::Extends<Stmt> {
  string_view name;
  Expr *superClass;
  vector<ClassMember *> members;
};

// The default specifier and a namespace specifier both leave `imported`
// empty (nothing is written between `import` and `from`); `isNamespace`
// tells them apart, a plain named specifier always has `imported` set.
struct ImportSpecifier : cursiv::Extends<cursiv::AstNode> {
  boolean isNamespace;
  string_view imported;
  string_view local;
};

struct ImportAttribute : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  string_view value;
};

struct ImportStmt : cursiv::Extends<Stmt> {
  vector<ImportSpecifier *> specifiers;
  string_view source;
  vector<ImportAttribute *> attributes;
};

struct ExportSpecifier : cursiv::Extends<cursiv::AstNode> {
  string_view exported;
  string_view local;
};

struct ExportStmt : cursiv::Extends<Stmt> {
  boolean isDefault;
  boolean isNamespace; // export * from "..."
  vector<ExportSpecifier *> specifiers;
  Stmt *declaration;  // export <VarDecl|FunctionDecl|ClassDecl>
  Expr *defaultExpr;  // export default <expr>
  string_view source; // re-export "from" clause, empty otherwise
  vector<ImportAttribute *> attributes;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  string_view hashbang; // empty when absent
  vector<Stmt *> body;
};

} // namespace javascript::ast
