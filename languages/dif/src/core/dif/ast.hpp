#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace dif::ast {

struct Pair : cursiv::Extends<cursiv::AstNode> {
  string_view first;
  string_view second;
};

struct Section : cursiv::Extends<cursiv::AstNode> {
  string_view topic;
  Pair* id;
  string label;
};

struct Header : cursiv::Extends<cursiv::AstNode> {
  vector<Section*> sections;
};

struct Datum : cursiv::Extends<cursiv::AstNode> {
  Pair* id;
  string value;
  boolean quoted;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  Header* header;
  vector<Datum*> items;
};

}  // namespace dif::ast
