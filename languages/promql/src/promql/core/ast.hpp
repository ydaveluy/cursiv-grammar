// The promql language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace promql::ast {

// One enum spans every operator this language reads: arithmetic, label
// matching (eq/ne/re/nre), comparison, set operators and the `@` modifier.
// A label matcher and a binary expression each use the subset that applies
// to them.
enum class Operator : std::uint8_t {
  add,
  subtract,
  multiply,
  divide,
  modulo,
  pow,
  atan2,
  trim_upper,
  trim_lower,
  eq,
  deq,
  ne,
  gt,
  lt,
  ge,
  le,
  re,
  nre,
  logical_and,
  logical_or,
  logical_unless,
  at,
};

struct Expression : cursiv::Extends<cursiv::AstNode> {};

struct NumberLit : cursiv::Extends<Expression> {
  float64 value;
};

struct DurationLit : cursiv::Extends<Expression> {
  string_view text;
};

struct StringLit : cursiv::Extends<Expression> {
  string_view value;
};

struct LabelMatcher : cursiv::Extends<cursiv::AstNode> {
  string_view name = "__name__";
  Operator op = Operator::eq;
  string_view value;
};

// One home for instantSelector, matrixSelector and offset: all three share
// the metric-name-then-braces prefix, and folding them avoids a redundant
// derivation (the same prefix reachable through three rules forks the parse
// at every occurrence).
struct Selector : cursiv::Extends<Expression> {
  string_view metric;  // empty for the bare `{...}` form
  vector<LabelMatcher*> matchers;
  Expression* range;
  Expression* offset;
};

struct FunctionCall : cursiv::Extends<Expression> {
  string_view name;
  vector<Expression*> args;
};

struct AggregationExpr : cursiv::Extends<Expression> {
  string_view op;
  vector<Expression*> args;
  boolean hasGrouping;
  boolean without;  // true = WITHOUT, false = BY (meaningful only if hasGrouping)
  vector<string_view> groupBy;
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

// `expr[range:step]offset?`, a subquery over a range vector.
struct Subquery : cursiv::Extends<Expression> {
  Expression* operand;
  Expression* range;
  Expression* step;
  Expression* offset;
};

struct TimestampBoundary : cursiv::Extends<Expression> {
  string_view name;
};
struct Timestamped : cursiv::Extends<Expression> {
  Expression* operand;
  Expression* timestamp;
  Expression* offset;
};

struct RangeModifier : cursiv::Extends<Expression> {
  Expression* operand;
  string_view name;
};

struct FillModifier : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Expression* value;
};

struct BinaryExpr : cursiv::Extends<Expression> {
  Expression* left;
  Operator op;
  boolean boolMod;      // the BOOL modifier -- comparisons only
  Grouping* matching;   // the vector-matching clause, or null
  vector<FillModifier*> fills;
  Expression* right;
};

struct Query : cursiv::Extends<cursiv::AstNode> {
  Expression* expression;
};

}  // namespace promql::ast
