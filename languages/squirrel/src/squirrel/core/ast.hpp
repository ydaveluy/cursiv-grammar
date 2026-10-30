// The Squirrel language: its AST. Plain structs -- the grammar names their
// members, and the generated builder fills them.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace squirrel::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

// ---- literals and names ----

// Kept as spelled: INT admits hex/octal/char-literal forms and FLOAT an
// exponent, so a single width-typed conversion would either reject or
// misread some of them (the char-literal form '\''[a-zA-Z]'\'' is not a
// digit run at all).
struct IntLit : cursiv::Extends<Expr> {
  string_view text;
};

struct FloatLit : cursiv::Extends<Expr> {
  string_view text;
};

// Unquoted only: the plain form's `\.` escapes and the verbatim form's `""`
// are not decoded, matching this codebase's json/ini string literals.
struct StringLit : cursiv::Extends<Expr> {
  string_view value;
  boolean verbatim = false;
};

struct BoolLit : cursiv::Extends<Expr> {
  boolean value;
};

struct NullLit : cursiv::Extends<Expr> {};

struct Ident : cursiv::Extends<Expr> {
  string_view name;
  boolean rootScoped;  // a leading `::`
};

// ---- compound expressions ----

struct ArrayExpr : cursiv::Extends<Expr> {
  vector<Expr*> items;
};

struct FunctionDecl;

struct KeyValue : cursiv::Extends<cursiv::AstNode> {
  std::variant<string_view, Expr*> key;  // a name, or a computed `[expr]` key
  Expr* value = nullptr;
};

struct TableExpr : cursiv::Extends<Expr> {
  vector<std::variant<KeyValue*, FunctionDecl*>> items;
};

struct Param : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* defaultValue = nullptr;
};

struct FunctionExpr : cursiv::Extends<Expr> {
  Expr* environment = nullptr;
  vector<Param*> params;
  boolean variadic;
  Stmt* body = nullptr;
};

struct LambdaExpr : cursiv::Extends<Expr> {
  Expr* environment = nullptr;
  vector<Param*> params;
  boolean variadic;
  Expr* body = nullptr;
};

struct ClassMember;

struct ClassExpr : cursiv::Extends<Expr> {
  vector<string_view> base;  // empty when there is no `extends`
  vector<ClassMember*> members;
};

struct MemberExpr : cursiv::Extends<Expr> {
  Expr* object = nullptr;
  string_view field;
};

struct CallExpr : cursiv::Extends<Expr> {
  Expr* callee = nullptr;
  vector<Expr*> args;
};

struct IndexExpr : cursiv::Extends<Expr> {
  Expr* object = nullptr;
  Expr* index = nullptr;
};

// `i++`, `i--`: a suffix of any postfix chain -- wider than Squirrel's own
// grammar, which only allows it after a bare name or a `[...]` index.
struct PostfixOpExpr : cursiv::Extends<Expr> {
  Expr* operand = nullptr;
  string_view op;
};

// `-x`, `!x`, `~x`, `typeof x`, `clone x`, `delete x`, `resume x`, and the
// prefix `++x`/`--x`. The operator's text, so a consumer does not have to
// know the set.
struct PrefixExpr : cursiv::Extends<Expr> {
  string_view op;
  Expr* operand = nullptr;
};

// Every binary level of the operator ladder, `instanceof`, and the comma
// operator: one class, `op` holding the operator's text.
struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  string_view op;
  Expr* right = nullptr;
};

struct CondExpr : cursiv::Extends<Expr> {
  Expr* cond = nullptr;
  Expr* thenValue = nullptr;
  Expr* elseValue = nullptr;
};

struct AssignExpr : cursiv::Extends<Expr> {
  Expr* target = nullptr;
  string_view op;
  Expr* value = nullptr;
};

// ---- statements ----

struct BlockStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> body;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};

struct ConstDecl : cursiv::Extends<Stmt> {
  string_view name;
  Expr* value = nullptr;  // one of the literal Expr classes
};

struct EnumItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* value = nullptr;  // optional literal
};

struct EnumDecl : cursiv::Extends<Stmt> {
  string_view name;
  vector<EnumItem*> items;
};

struct VarItem : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expr* init = nullptr;
};

struct LocalVarDecl : cursiv::Extends<Stmt> {
  vector<VarItem*> vars;
};

// `function name(...) {...}`: a statement, a class method, or a table item
// -- one class for all three, since Squirrel spells them the same way.
struct FunctionDecl : cursiv::Extends<Stmt> {
  Expr* environment = nullptr;
  vector<string_view> path;  // `Foo::bar` splits into two segments
  vector<Param*> params;
  boolean variadic;
  Stmt* body = nullptr;
};

// `local function f(...) {...}`.
struct LocalFunctionDecl : cursiv::Extends<Stmt> {
  FunctionDecl* func = nullptr;
};

// The body of a bare `;`.
struct EmptyStmt : cursiv::Extends<Stmt> {};

struct ConstructorDecl : cursiv::Extends<cursiv::AstNode> {
  Expr* environment = nullptr;
  vector<Param*> params;
  boolean variadic;
  Stmt* body = nullptr;
};

struct ClassMember : cursiv::Extends<cursiv::AstNode> {
  vector<KeyValue*> attrs;  // `</ ... />`, empty when absent
  boolean isStatic;
  std::variant<ConstructorDecl*, FunctionDecl*, KeyValue*> member;
};

struct ClassDecl : cursiv::Extends<Stmt> {
  vector<string_view> name;
  vector<string_view> base;  // empty when there is no `extends`
  vector<ClassMember*> members;
};

struct ForStmt : cursiv::Extends<Stmt> {
  Stmt* init = nullptr;  // a LocalVarDecl or an ExprStmt, or null
  Expr* cond = nullptr;
  Expr* step = nullptr;
  Stmt* body = nullptr;
};

struct ForeachStmt : cursiv::Extends<Stmt> {
  string_view indexName;  // empty when the `k,` part is absent
  string_view valueName;
  Expr* collection = nullptr;
  Stmt* body = nullptr;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Expr* cond = nullptr;
  Stmt* body = nullptr;
};

struct DoWhileStmt : cursiv::Extends<Stmt> {
  Stmt* body = nullptr;
  Expr* cond = nullptr;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Expr* cond = nullptr;
  Stmt* thenBranch = nullptr;
  Stmt* elseBranch = nullptr;
};

struct SwitchCase : cursiv::Extends<cursiv::AstNode> {
  Expr* value = nullptr;
  vector<Stmt*> body;
};

struct DefaultCase : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> body;
};

struct SwitchStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
  vector<SwitchCase*> cases;
  DefaultCase* defaultCase = nullptr;
};

struct TryStmt : cursiv::Extends<Stmt> {
  Stmt* body = nullptr;
  string_view catchName;
  Stmt* catchBody = nullptr;
};

struct ReturnStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};

struct BreakStmt : cursiv::Extends<Stmt> {};
struct ContinueStmt : cursiv::Extends<Stmt> {};

struct YieldStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};

struct ThrowStmt : cursiv::Extends<Stmt> {
  Expr* value = nullptr;
};

struct Chunk : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> body;
};

}  // namespace squirrel::ast
