// The javadoc language: its AST. A Documentation holds an interleaved
// description of text runs and inline tags, followed by block tags; each
// block tag holds the same interleaved shape for its own content.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace javadoc::ast {

struct InlineTag;
struct InlineBrace;
using Item = std::variant<std::string_view, InlineTag *, InlineBrace *>;

struct InlineTag : cursiv::Extends<cursiv::AstNode> {
  string_view open;     // actual punctuation, empty if recovery inserted it
  string_view name;
  string_view content;  // raw families: code, literal, snippet, unknown...
  bool structured = false;
  vector<Item> parts;  // structured bodies/labels; use raw_inline_content for text
  string_view close;
};

// Plain balanced braces remain source syntax; tags inside them still have
// real children. No group nodes are made inside raw code/literal bodies.
struct InlineBrace : cursiv::Extends<cursiv::AstNode> {
  string_view open;
  vector<Item> parts;
  string_view close;
};

struct BlockTag : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Item> content;
};

struct Documentation : cursiv::Extends<cursiv::AstNode> {
  bool wrapped = false;
  vector<Item> description;
  vector<BlockTag *> tags;
};

} // namespace javadoc::ast
