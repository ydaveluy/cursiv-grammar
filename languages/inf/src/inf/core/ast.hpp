#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace inf::ast {

struct Item : cursiv::Extends<cursiv::AstNode> {
  string_view raw;
};

struct Line : cursiv::Extends<cursiv::AstNode> {
  Item* key;
  vector<Item*> fields;
};

struct Section : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view closing;
  vector<Line*> lines;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  boolean has_bom;
  vector<Line*> preamble;
  vector<Section*> sections;
  string_view trailer;
};

}  // namespace inf::ast
