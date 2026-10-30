// The pl0 language: its AST. Names, not references -- a nested procedure may
// read an enclosing block's declarations, and no example needs the link
// resolved to be useful.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pl0::ast {

struct ConstDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value; // exact decimal lexeme; no parser-imposed integer bound
};

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct NameExpr : cursiv::Extends<Expr> {
  string_view name;
};

struct NumberExpr : cursiv::Extends<Expr> {
  string_view value; // exact decimal lexeme; no parser-imposed integer bound
};

struct GroupedExpr : cursiv::Extends<Expr> {
  Expr* expression;
};

enum class Operator : std::uint8_t { add, subtract, multiply, divide };

// The leading sign of an expression: attaches to its first term only, never
// to an operand after `*`, `/`, `+` or `-`.
struct UnaryExpr : cursiv::Extends<Expr> {
  Operator op;
  Expr* operand;
};

struct BinaryExpr : cursiv::Extends<Expr> {
  Expr* left;
  Operator op;
  Expr* right;
};

enum class RelOp : std::uint8_t { eq, ne, lt, le, gt, ge };

struct Cond : cursiv::Extends<cursiv::AstNode> {};

struct OddCond : cursiv::Extends<Cond> {
  Expr* operand;
};

struct CompareCond : cursiv::Extends<Cond> {
  Expr* left;
  RelOp op;
  Expr* right;
};

struct Stmt : cursiv::Extends<cursiv::AstNode> {};

struct AssignStmt : cursiv::Extends<Stmt> {
  string_view name;
  Expr* value;
};

struct CallStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct WriteStmt : cursiv::Extends<Stmt> {
  string_view name;
};

struct ReadStmt : cursiv::Extends<Stmt> {
  string_view name;
};

// `statement` may derive nothing (a dangling `;`, an empty procedure body):
// a concrete node marks that choice instead of a null slot.
struct EmptyStmt : cursiv::Extends<Stmt> {};

struct PrintStmt : cursiv::Extends<Stmt> {
  Expr* value;
};

struct CompoundStmt : cursiv::Extends<Stmt> {
  vector<Stmt*> statements;
};

struct IfStmt : cursiv::Extends<Stmt> {
  Cond* condition;
  Stmt* then;
};

struct WhileStmt : cursiv::Extends<Stmt> {
  Cond* condition;
  Stmt* body;
};

struct Block;

struct Procedure : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Block* body;
};

struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<ConstDecl*> consts;
  vector<string_view> vars;
  vector<Procedure*> procedures;
  Stmt* body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  Block* block;
};

}  // namespace pl0::ast
