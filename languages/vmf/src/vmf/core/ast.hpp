// The vmf language: its AST. A KeyValues (Valve KeyValues / VMF) document is
// a tree of key/value pairs where a key's payload is either bare text values
// or a nested block of more key/value pairs -- never both.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace vmf::ast {

struct KeyValue : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  boolean keyQuoted;
  boolean valueQuoted;
  optional<string_view> value;   // set when the payload is bare text
  vector<KeyValue*> children;    // set when the payload is a {...} block
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<KeyValue*> entries;
};

}  // namespace vmf::ast
