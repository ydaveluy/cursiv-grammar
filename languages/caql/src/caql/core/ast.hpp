// The caql language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace caql::ast {

// Arithmetic, comparison and set operators, plus unary sign: one enum
// spans them, and each construct uses the subset that applies to it.
enum class Operator : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  modulo,
  pow,
  deq,
  ne,
  gt,
  lt,
  ge,
  le,
  logical_and,
  logical_or,
  logical_unless,
};

struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct NumberLit : cursiv::Extends<Expression> {
  float64 value;
};

struct StringLit : cursiv::Extends<Expression> {
  string_view value;
};

struct UuidLit : cursiv::Extends<Expression> {
  string_view value;
};

// One argument of a `name(...)` call: a literal, with the `limit=` marker
// legal on every argument but the first.
struct CallArg : cursiv::Extends<cursiv::AstNode> {
  boolean limit;
  Expression* value;
};

struct CallExpr : cursiv::Extends<Expression> {
  string_view name;
  vector<CallArg*> args;
};

// `name{ e1, e2, ... }`.
struct BraceExpr : cursiv::Extends<Expression> {
  string_view name;
  vector<Expression*> operands;
};

// The `on(...)`/`ignoring(...)` vector-matching clause, plus an optional
// `group_left`/`group_right` side.
struct Grouping : cursiv::Extends<cursiv::AstNode> {
  boolean ignoring;  // false = ON, true = IGNORING
  vector<string_view> labels;
  boolean hasSide;
  boolean groupRight;  // true = GROUP_RIGHT, false = GROUP_LEFT (only if hasSide)
  vector<string_view> sideLabels;
};

struct UnaryExpr : cursiv::Extends<Expression> {
  Operator op;  // add or subtract
  Expression* operand;
};

// `expr[range:step]offset?`, a subquery over a range.
struct Subquery : cursiv::Extends<Expression> {
  Expression* operand;
  string_view range;
  string_view step;    // empty when the query has no downsampling step
  string_view offset;
};

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  Operator op;
  boolean boolMod;      // the BOOL modifier -- comparisons only
  Grouping* matching;   // the vector-matching clause, or null
  Expression* right;
};

// `operand | name(args)`, the metrics-aggregation pipe. Each arg is a
// literal or a full expression.
struct PipeExpr : cursiv::Extends<Expression> {
  Expression* operand;
  string_view name;
  vector<Expression*> args;
};

struct Query : cursiv::Extends<cursiv::AstNode> {
  Expression* expression;
};

}  // namespace caql::ast
