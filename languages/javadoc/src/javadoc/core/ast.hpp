// The javadoc language: its AST. A Documentation holds an interleaved
// description of text runs and inline tags, followed by block tags; each
// block tag holds the same interleaved shape for its own content.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace javadoc::ast {

struct InlineTag : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view content;
};

using Item = std::variant<std::string_view, InlineTag *>;

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
