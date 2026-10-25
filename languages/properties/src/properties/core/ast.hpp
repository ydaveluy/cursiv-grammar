// The properties language: its AST. A document is an ordered list of rows,
// each either a key/value entry or a comment -- order matters for a
// validator that reasons about which entry wins.
#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace properties::ast {

struct Entry : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  // Unset when the line has no delimiter at all (a bare key); empty text
  // when the delimiter is present but nothing follows it.
  optional<string_view> value;
};

struct Comment : cursiv::Extends<cursiv::AstNode> {
  string_view text;  // includes the leading '#' or '!'
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<std::variant<Entry*, Comment*>> rows;
};

}  // namespace properties::ast
