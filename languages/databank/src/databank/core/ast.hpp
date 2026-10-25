// The databank language: its AST. A document is one time series: an
// optional leading type code, its two date bounds, and its samples.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace databank::ast {

// A sample is a number, or 'NA' for a missing reading (monostate).
using Sample = std::variant<std::monostate, double>;

struct Series : cursiv::Extends<cursiv::AstNode> {
  optional<double> datatype;  // present: -1, -4 or -12, a dated series
  double from = 0;
  double to = 0;
  vector<Sample> samples;
};

}  // namespace databank::ast
