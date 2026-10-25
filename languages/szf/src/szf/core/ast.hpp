#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace szf::ast {
struct Element : cursiv::Extends<cursiv::AstNode> {};
struct Header : cursiv::Extends<Element> {
  string_view name;
};
struct Opcode : cursiv::Extends<Element> {
  string_view key;
  string_view value;
};
struct Definition : cursiv::Extends<Element> {
  string_view name;
  string_view value;
};
struct Include : cursiv::Extends<Element> {
  string_view path;
};
struct UnknownDirective : cursiv::Extends<Element> {
  string_view text;
};
struct File : cursiv::Extends<cursiv::AstNode> {
  vector<Element*> elements;
};
}  // namespace szf::ast
