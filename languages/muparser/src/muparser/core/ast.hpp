// The muparser language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace muparser::ast {

struct Expression : cursiv::Extends<cursiv::AstNode> {};

enum class Operator : std::uint8_t {
  pow,
  mul,
  div,
  add,
  sub,
  lte,
  gte,
  lt,
  gt,
  eq,
  neq,
  and_,
  or_,
};

enum class AssignOp : std::uint8_t { assign, add, sub, mul, div };

enum class Constant : std::uint8_t { e, pi };

struct NumberLiteral : cursiv::Extends<Expression> {
  float64 value;
};

struct BoolLiteral : cursiv::Extends<Expression> {
  bool value;
};

struct PredefinedConstant : cursiv::Extends<Expression> {
  Constant kind;
};

struct VariableRef : cursiv::Extends<Expression> {
  string_view name;
};

struct FunctionCall : cursiv::Extends<Expression> {
  string_view name;
  vector<Expression*> args;
};

struct AssignExpr : cursiv::Extends<Expression> {
  Expression* target;
  AssignOp op;
  Expression* value;
};

struct UnaryExpr : cursiv::Extends<Expression> {
  Operator op;
  Expression* operand;
};

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  Operator op;
  Expression* right;
};

struct CondExpr : cursiv::Extends<Expression> {
  Expression* cond;
  Expression* thenValue;
  Expression* elseValue;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Expression*> exprs;
};

}  // namespace muparser::ast
