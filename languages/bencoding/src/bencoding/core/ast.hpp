// The bencoding language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace bencoding::ast {

struct Integer : cursiv::Extends<cursiv::AstNode> {
  string_view text; // unbounded decimal spelling between i and e
};
struct ByteString : cursiv::Extends<cursiv::AstNode> {
  string_view length; // decimal count, before the colon
  string_view value;  // original payload bytes, including NUL
};
struct List;
struct Dict;

// Namespace scope: the AstNode vocabulary (string_view, vector) is a
// member alias brought in by Extends<>, out of reach here.
using Value = std::variant<Integer *, ByteString *, List *, Dict *>;

struct List : cursiv::Extends<cursiv::AstNode> {
  vector<Value> items;
};

struct Entry : cursiv::Extends<cursiv::AstNode> {
  ByteString *key;
  Value value;
};

struct Dict : cursiv::Extends<cursiv::AstNode> {
  vector<Entry *> entries;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Value> values;
};

} // namespace bencoding::ast
