// The tsv language: its AST. A document is a header row and the data rows
// below it; a row is just its fields, in source order.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tsv::ast {

struct Row : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> fields;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  Row* header;
  vector<Row*> rows;
};

}  // namespace tsv::ast
