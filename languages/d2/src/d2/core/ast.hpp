#pragma once

#include <cstdint>
#include <optional>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace d2::ast {
struct Node : cursiv::Extends<cursiv::AstNode> {};
struct TextPart : cursiv::Extends<Node> {};
struct Chunk : cursiv::Extends<TextPart> {
  string_view raw;
};
struct KeyPath;
struct Substitution : cursiv::Extends<TextPart> {
  KeyPath* path;
  bool spread;
};
struct Text : cursiv::Extends<Node> {
  vector<TextPart*> parts;
};
struct QuotedText : cursiv::Extends<Text> {};
struct SingleText : cursiv::Extends<Text> {};
struct BlockText : cursiv::Extends<Text> {};
struct KeyPath : cursiv::Extends<Node> {
  vector<Text*> parts;
};
struct Import : cursiv::Extends<Node> {
  bool spread;
  string_view prefix;
  KeyPath* path;
};
struct Hop : cursiv::Extends<Node> {
  string_view op;
  KeyPath* target;
};
struct Map : cursiv::Extends<Node> {
  vector<Node*> statements;
};
struct Array : cursiv::Extends<Node> {
  vector<Node*> values;
};
struct Key : cursiv::Extends<Node> {
  bool filter;
  bool negated;
  bool grouped;
  bool colon;
  KeyPath* scope;
  KeyPath* path;
  vector<Hop*> hops;
  std::optional<string_view> index;
  KeyPath* property;
  Node* value;
  Map* block;
};
struct Diagram : cursiv::Extends<cursiv::AstNode> {
  vector<Node*> statements;
};
}  // namespace d2::ast
