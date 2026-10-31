// The Algol 60 language: its AST. Expressions, statements and declarations
// each root a small polymorphic family; every name is text, never a
// reference -- most of them name standard procedures no document declares.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace algol60::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};
struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Decl : cursiv::Extends<cursiv::AstNode> {};

struct NameRef : cursiv::Extends<Expr> {
  string_view name;
};

struct NumberLiteral : cursiv::Extends<Expr> {
  double value = 0;
};

struct LogicalLiteral : cursiv::Extends<Expr> {
  boolean value = false;
};

struct StringLiteral : cursiv::Extends<Expr> {
  string_view text;
};

enum class UnOp { plus, minus, lnot };
enum class BinOp {
  power, mul, div, add, sub,
  lt, le, eq, ne, gt, ge,
  land, lor, implies, equiv, intdiv,
};

struct UnaryExpr : cursiv::Extends<Expr> {
  UnOp op = UnOp::plus;
  Expr* operand = nullptr;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left = nullptr;
  BinOp op = BinOp::add;
  Expr* right = nullptr;
};

// The `if A then B else C` expression form, shared by arithmetic, Boolean
// and designational expressions: same shape, three different operand kinds.
struct IfExpr : cursiv::Extends<Expr> {
  Expr* cond = nullptr;
  Expr* then_branch = nullptr;
  Expr* else_branch = nullptr;
};

// A subscripted variable or a switch designator: a name plus one or more
// index expressions.
struct IndexExpr : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> indices;
};

struct CallExpr : cursiv::Extends<Expr> {
  string_view callee;
  vector<Expr*> args;
};

// Labels can prefix list entries and the single statement of an if/for.
struct StmtEntry : cursiv::Extends<Stmt> {
  vector<string_view> labels;
  Stmt* stmt = nullptr;
};

struct EmptyStmt : cursiv::Extends<Stmt> {};

struct AssignStmt : cursiv::Extends<Stmt> {
  vector<Expr*> targets;  // NameRef or IndexExpr
  Expr* value = nullptr;
};

struct GotoStmt : cursiv::Extends<Stmt> {
  Expr* target = nullptr;
};

struct CallStmt : cursiv::Extends<Stmt> {
  string_view callee;
  vector<Expr*> args;
};

struct IfStmt : cursiv::Extends<Stmt> {
  boolean hasElse = false;
  Expr* cond = nullptr;
  Stmt* then_branch = nullptr;
  Stmt* else_branch = nullptr;
};

struct ForSimple : cursiv::Extends<cursiv::AstNode> {
  Expr* value = nullptr;
};

struct ForStepUntil : cursiv::Extends<cursiv::AstNode> {
  Expr* from = nullptr;
  Expr* step = nullptr;
  Expr* to = nullptr;
};

struct ForWhile : cursiv::Extends<cursiv::AstNode> {
  Expr* value = nullptr;
  Expr* cond = nullptr;
};

// Namespace scope, so the AstNode vocabulary is out of reach and the std
// spelling is the only one.
using ForItem = std::variant<ForSimple*, ForStepUntil*, ForWhile*>;

struct ForStmt : cursiv::Extends<Stmt> {
  string_view var;
  vector<ForItem> items;
  Stmt* body = nullptr;
};

struct Block : cursiv::Extends<Stmt> {
  vector<string_view> labels;  // labels of the outer program
  vector<Decl*> decls;
  vector<StmtEntry*> statements;
};

struct TypeDecl : cursiv::Extends<Decl> {
  boolean own_ = false;
  string_view type_name;
  vector<string_view> names;
};

struct BoundPair : cursiv::Extends<cursiv::AstNode> {
  Expr* lower = nullptr;
  Expr* upper = nullptr;
};

struct ArraySegment : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> names;
  vector<BoundPair*> bounds;
};

struct ArrayDecl : cursiv::Extends<Decl> {
  boolean own_ = false;
  string_view type_name;  // empty: no type before `array`
  vector<ArraySegment*> segments;
};

struct SwitchDecl : cursiv::Extends<Decl> {
  string_view name;
  vector<Expr*> items;
};

// One `specifier identifier_list` clause of a procedure heading, e.g.
// `integer n` or `array a,b`.
struct Specifier : cursiv::Extends<cursiv::AstNode> {
  string_view kind;       // string/array/label/switch/procedure/a type name
  string_view type_name;  // empty when the specifier has none
  vector<string_view> names;
};

struct ProcDecl : cursiv::Extends<Decl> {
  string_view type_name;  // empty: an untyped procedure
  string_view name;
  vector<string_view> params;
  vector<string_view> value_params;
  vector<Specifier*> specs;
  Stmt* body = nullptr;
};

}  // namespace algol60::ast
