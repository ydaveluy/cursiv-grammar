#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace xyz::ast {

struct Atom : cursiv::Extends<cursiv::AstNode> {
  string_view element;
  float64 x;
  float64 y;
  float64 z;
  vector<string_view> extra;
};

struct Frame : cursiv::Extends<cursiv::AstNode> {
  string_view count;
  string_view description;
  vector<Atom*> atoms;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Frame*> frames;
};

}  // namespace xyz::ast
