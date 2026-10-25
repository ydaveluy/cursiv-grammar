// The cookie language: its AST -- a document is a flat, ordered list of
// attribute/value pairs, the shape of an HTTP Cookie or Set-Cookie header.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace cookie::ast {

struct Pair : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  optional<string_view> value;  // absent for a bare attribute, e.g. Secure
  bool quoted = false;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Pair*> pairs;
};

}  // namespace cookie::ast
