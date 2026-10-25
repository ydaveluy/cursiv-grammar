// The arithmetic language: its AST.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace arithmetic::ast {

// The operation, not the text that denotes it: each operator token hands its
// constant over.
enum class Operator : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  exponent,
};

enum class RelOp : std::uint8_t {
  eq,
  gt,
  lt,
};

struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct BinaryExpression : cursiv::Extends<Expression> {
  Expression* left;
  Operator op;
  Expression* right;
};

// The prefix `+`/`-` chain: each sign wraps the next reading, down to an atom.
struct UnaryExpression : cursiv::Extends<Expression> {
  Operator op;
  Expression* operand;
};

struct GroupedExpression : cursiv::Extends<Expression> {
  Expression* expression;
};

struct NumberLiteral : cursiv::Extends<Expression> {
  float64 value;
};

struct VariableExpression : cursiv::Extends<Expression> {
  string_view name;
};

struct Equation : cursiv::Extends<cursiv::AstNode> {
  Expression* left;
  RelOp op;
  Expression* right;
};

struct File : cursiv::Extends<cursiv::AstNode> {
  vector<Equation*> equations;
};

}  // namespace arithmetic::ast
