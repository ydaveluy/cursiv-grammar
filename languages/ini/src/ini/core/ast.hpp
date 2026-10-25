#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ini::ast {
struct Line : cursiv::Extends<cursiv::AstNode> {
  string_view indentation;
  string_view ending;
};
struct SectionHeader : cursiv::Extends<Line> {
  string_view open;
  string_view name;
  string_view close;
  string_view suffix;
};
struct Option : cursiv::Extends<Line> {
  string_view key;
  string_view separator;
  string_view value;
};
struct Comment : cursiv::Extends<Line> {
  string_view text;
};
struct Blank : cursiv::Extends<Line> {};
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};
} // namespace ini::ast
