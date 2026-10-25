// The gvpr language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace gvpr::ast {

struct Stmt : cursiv::Extends<cursiv::AstNode> {};
struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct TypeRef : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  boolean pointer;
};

enum class AssignOp : std::uint8_t { set, add, sub, mul, div };

struct DclItem : cursiv::Extends<cursiv::AstNode> {
  optional<AssignOp> initOp;
  string_view name;
  TypeRef* keyType;
  Expr* init;
};

struct Formal : cursiv::Extends<cursiv::AstNode> {
  TypeRef* type;
  string_view name;
};

struct Case : cursiv::Extends<cursiv::AstNode> {
  boolean isDefault;
  Expr* value;
};

struct SwitchItem : cursiv::Extends<cursiv::AstNode> {
  vector<Case*> cases;
  vector<Stmt*> statements;
};

struct Action : cursiv::Extends<cursiv::AstNode> {
  string_view label;
  vector<Stmt*> statements;
};

// Shared by a predicate's body and, for N/E, its guard.
struct Body : cursiv::Extends<cursiv::AstNode> {
  vector<Stmt*> statements;
  vector<Action*> actions;
};

enum class PredKind : std::uint8_t { begin_, beg_g, end_g, end_ };
enum class GuardKind : std::uint8_t { node_, edge_ };

struct Pred : cursiv::Extends<cursiv::AstNode> {};

struct SimplePred : cursiv::Extends<Pred> {
  PredKind kind;
  Body* body;
};

struct GuardedPred : cursiv::Extends<Pred> {
  GuardKind kind;
  Body* guard;
  Body* body;
};

struct Preds : cursiv::Extends<cursiv::AstNode> {
  vector<Pred*> items;
};

// Statements

struct Block : cursiv::Extends<Stmt> {
  vector<Stmt*> statements;
};

struct VarDecl : cursiv::Extends<Stmt> {
  boolean isStatic;
  TypeRef* type;
  vector<DclItem*> items;
};

struct FuncDecl : cursiv::Extends<Stmt> {
  boolean isStatic;
  TypeRef* returnType;
  string_view name;
  vector<Formal*> formals;
  vector<Stmt*> statements;
};

struct ExprStmt : cursiv::Extends<Stmt> {
  Expr* expr;
};

struct If : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* then_;
  Stmt* else_;
};

// gvpr's `for (variable) statement` iterates the collection a variable's
// runtime value names -- semantics this AST does not evaluate, only parses.
struct ForVar : cursiv::Extends<Stmt> {
  Expr* var;
  Stmt* body;
};

struct ForC : cursiv::Extends<Stmt> {
  Expr* init;
  Expr* cond;
  Expr* step;
  Stmt* body;
};

struct ForR : cursiv::Extends<Stmt> {
  Expr* var;
  Stmt* body;
};

struct Unset : cursiv::Extends<Stmt> {
  string_view name;
  Expr* value;
};

struct While : cursiv::Extends<Stmt> {
  Expr* cond;
  Stmt* body;
};

struct Switch : cursiv::Extends<Stmt> {
  Expr* subject;
  vector<SwitchItem*> items;
};

struct Break : cursiv::Extends<Stmt> {
  Expr* value;
};

struct Continue : cursiv::Extends<Stmt> {
  Expr* value;
};

struct Return : cursiv::Extends<Stmt> {
  Expr* value;
};

// Expressions

struct Var : cursiv::Extends<Expr> {
  string_view name;  // an ID, or "$" for the current-object variable
  Expr* index;
  vector<string_view> members;
};


struct Assign : cursiv::Extends<Expr> {
  Var* target;
  AssignOp op;
  Expr* value;
};

enum class IncDecOp : std::uint8_t { inc, dec };

struct PrefixIncDec : cursiv::Extends<Expr> {
  IncDecOp op;
  Var* var;
};

struct PostfixIncDec : cursiv::Extends<Expr> {
  Var* var;
  IncDecOp op;
};

enum class UnaryOp : std::uint8_t { not_, bnot, neg, pos };

struct Unary : cursiv::Extends<Expr> {
  UnaryOp op;
  Expr* operand;
};

// The field-count operator; its operand is a bare name.
struct Cardinality : cursiv::Extends<Expr> {
  string_view name;
};

struct AddressOf : cursiv::Extends<Expr> {
  Var* var;
};

enum class BinOp : std::uint8_t {
  mul,
  div,
  mod,
  add,
  sub,
  in_,
  lsh,
  rsh,
  lt,
  gt,
  le,
  ge,
  eq,
  ne,
  band,
  bxor,
  bor,
  land,
  lor,
  comma
};

struct Binary : cursiv::Extends<Expr> {
  Expr* left;
  BinOp op;
  Expr* right;
};

struct Ternary : cursiv::Extends<Expr> {
  Expr* cond;
  Expr* then_;
  Expr* else_;
};

struct ArrayCall : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
};

struct FuncCall : cursiv::Extends<Expr> {
  string_view name;
  vector<Expr*> args;
};

enum class BuiltinKind : std::uint8_t {
  gsub,
  sub,
  substr,
  exit_,
  rand_,
  srand_,
  procedure,
  print_,
  printf_,
  query,
  sprintf_,
  scanf_,
  sscanf_
};

struct BuiltinCall : cursiv::Extends<Expr> {
  BuiltinKind kind;
  vector<Expr*> args;
};

struct SplitCall : cursiv::Extends<Expr> {
  boolean tokens;
  Expr* subject;
  string_view arrayName;
  Expr* separator;
};

struct Cast : cursiv::Extends<Expr> {
  TypeRef* type;
  Expr* operand;
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

struct StringLit : cursiv::Extends<Expr> {
  string_view text;
};

}  // namespace gvpr::ast
