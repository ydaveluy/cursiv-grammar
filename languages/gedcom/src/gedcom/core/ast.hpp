#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace gedcom::ast {

struct Line : cursiv::Extends<cursiv::AstNode> {
  string_view level;
  string_view name;
  string_view tag;
  string_view value;
  boolean hasValue = false;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Line *> lines;
  boolean bom = false;
};

} // namespace gedcom::ast
