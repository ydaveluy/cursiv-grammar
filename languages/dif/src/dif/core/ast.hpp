#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace dif::ast {

struct Pair : cursiv::Extends<cursiv::AstNode> {
  string_view first;
  string_view second;
};

struct Section : cursiv::Extends<cursiv::AstNode> {
  string_view topic;
  Pair* id;
  string_view label;
};

struct Header : cursiv::Extends<cursiv::AstNode> {
  vector<Section*> sections;
};

struct Datum : cursiv::Extends<cursiv::AstNode> {
  Pair* id;
  string_view value;
  boolean quoted;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  Header* header;
  vector<Datum*> items;
};

}  // namespace dif::ast
