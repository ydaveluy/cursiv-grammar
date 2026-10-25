// CSV rows remain in source order. A consumer decides whether the first row
// contains column names or ordinary data.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace csv::ast {

struct Field : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};

struct Row : cursiv::Extends<cursiv::AstNode> {
  vector<Field*> fields;
};

struct Doc : cursiv::Extends<cursiv::AstNode> {
  vector<Row*> rows;
};

}  // namespace csv::ast
