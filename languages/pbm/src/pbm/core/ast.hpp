#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
namespace pbm::ast {
struct Image : cursiv::Extends<cursiv::AstNode> {
  string_view magic;
  string_view width;
  string_view height;
  string_view separator;
  string_view raster;
  vector<boolean> table;
};
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Image*> images;
  string_view trailing;
};
}  // namespace pbm::ast
