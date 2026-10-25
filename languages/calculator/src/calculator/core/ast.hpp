// The calculator language: its AST.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace calculator::ast {

// The operation, not the text that denotes it: each operator token hands its
// constant over. Reused for the unary `+`/`-` sign (add/subtract).
enum class Operator : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  power,
};

enum class RelOp : std::uint8_t {
  eq,
  gt,
  lt,
};

enum class Constant : std::uint8_t {
  pi,
  euler,
  i,
};

enum class Func : std::uint8_t {
  cos,
  sin,
  tan,
  acos,
  asin,
  atan,
  log,
  ln,
  sqrt,
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

// The raw text, not a computed value: a scientific number's exponent may
// itself carry a decimal part ("12.3e13.2"), which the language accepts but
// no arithmetic value spells.
struct NumberLiteral : cursiv::Extends<Expression> {
  string_view value;
};

struct VariableExpression : cursiv::Extends<Expression> {
  string_view name;
};

struct ConstantExpression : cursiv::Extends<Expression> {
  Constant value;
};

struct FunctionCall : cursiv::Extends<Expression> {
  Func func;
  vector<Expression*> args;
};

struct Equation : cursiv::Extends<cursiv::AstNode> {
  Expression* left;
  RelOp op;
  Expression* right;
};

}  // namespace calculator::ast
