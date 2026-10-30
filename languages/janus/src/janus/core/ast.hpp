// The janus language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace janus::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct VarDecl : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<int64> size;
};

struct Procedure : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Statement*> statements;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<VarDecl*> decls;
  vector<Procedure*> procedures;
};

struct IfStmt : cursiv::Extends<Statement> {
  Expression* test;
  vector<Statement*> thenStatements;
  vector<Statement*> elseStatements;
  Expression* assertion;
};

struct DoStmt : cursiv::Extends<Statement> {
  Expression* from;
  vector<Statement*> doStatements;
  vector<Statement*> loopStatements;
  Expression* until;
};

enum class CallKind : std::uint8_t { call, uncall };

struct CallStmt : cursiv::Extends<Statement> {
  CallKind kind;
  reference<Procedure> target;
};

struct ReadStmt : cursiv::Extends<Statement> {
  string_view name;
  Expression* index;
};

struct WriteStmt : cursiv::Extends<Statement> {
  string_view name;
};

// A variable, plain or subscripted -- also an Expression, since minexp reads
// one directly.
struct Lvalue : cursiv::Extends<Expression> {
  string_view name;
  Expression* index;
};

enum class ModOp : std::uint8_t { plus_eq, minus_eq, bang_eq, exchange };

struct ModStmt : cursiv::Extends<Statement> {
  Lvalue* target;
  ModOp op;
  Expression* value;
};

struct SwapStmt : cursiv::Extends<Statement> {
  Lvalue* target;
  Lvalue* other;
};

struct ParenExpr : cursiv::Extends<Expression> {
  Expression* expression;
};

enum class UnaryOp : std::uint8_t { negate, complement };

// minexp's '-'/'~' branches recurse on the whole expression, not on
// minexp: the operand can itself be a binary chain.
struct UnaryExpr : cursiv::Extends<Expression> {
  UnaryOp op;
  Expression* operand;
};

struct ConstantExpr : cursiv::Extends<Expression> {
  int64 value;
};

enum class BinOp : std::uint8_t {
  plus,
  minus,
  bang,
  less,
  greater,
  amp,
  pipe,
  equal,
  hash,
  less_equal,
  greater_equal,
  star,
  slash,
  backslash,
};

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  BinOp op;
  Expression* right;
};

}  // namespace janus::ast
