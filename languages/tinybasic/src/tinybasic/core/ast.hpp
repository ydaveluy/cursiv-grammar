// The tinybasic language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tinybasic::ast {

struct Statement : cursiv::Extends<cursiv::AstNode> {};
struct Expression : cursiv::Extends<cursiv::AstNode> {};

// A single-letter variable, or a string used where a name is expected (an
// INPUT prompt, or an assignment target).
struct VarRef : cursiv::Extends<Expression> {
  string_view name;
};

struct StringLit : cursiv::Extends<Expression> {
  string_view value;
};

struct NumberLit : cursiv::Extends<Expression> {
  int32 value;
};

struct Negate : cursiv::Extends<Expression> {
  Expression* operand;
};

enum class Operator : std::uint8_t { add, subtract, multiply, divide };

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  Operator op;
  Expression* right;
};

enum class RelOp : std::uint8_t { lt, le, ne, gt, ge, eq };

struct PrintStmt : cursiv::Extends<Statement> {
  vector<Expression*> items;
};

struct IfStmt : cursiv::Extends<Statement> {
  Expression* left;
  RelOp op;
  Expression* right;
  Statement* then;
};

// GOTO takes a line number, never a general expression.
struct GotoStmt : cursiv::Extends<Statement> {
  int32 target;
};

struct InputStmt : cursiv::Extends<Statement> {
  vector<Expression*> items;
};

struct LetStmt : cursiv::Extends<Statement> {
  Expression* target;
  Expression* value;
};

// Unlike GOTO, GOSUB takes a general expression.
struct GosubStmt : cursiv::Extends<Statement> {
  Expression* target;
};

struct ReturnStmt : cursiv::Extends<Statement> {};
struct ClearStmt : cursiv::Extends<Statement> {};
struct ListStmt : cursiv::Extends<Statement> {};
struct RunStmt : cursiv::Extends<Statement> {};
struct EndStmt : cursiv::Extends<Statement> {};

struct Line : cursiv::Extends<cursiv::AstNode> {
  optional<int32> number;
  Statement* statement;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};

}  // namespace tinybasic::ast
