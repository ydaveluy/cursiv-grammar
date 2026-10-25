#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
namespace stl::ast {
struct Triple : cursiv::Extends<cursiv::AstNode> {
  string_view i;
  string_view j;
  string_view k;
  bool binary = false;
};
struct Facet : cursiv::Extends<cursiv::AstNode> {
  Triple *normal;
  vector<Triple *> vertices;
  uint32 attribute = 0;
};
struct Solid : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view endName;
  bool hasEnd = false;
  vector<Facet *> facets;
};
struct Document : cursiv::Extends<cursiv::AstNode> {
  bool binary = false;
  string_view header;
  string_view countBytes;
  vector<Solid *> solids;
  string_view trailing;
};
} // namespace stl::ast
