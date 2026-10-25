// The desktopentry language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace desktopentry::ast {

struct Locale : cursiv::Extends<cursiv::AstNode> {
  string_view language;
  string_view country;
  string_view encoding;
  string_view modifier;
};

struct Entry : cursiv::Extends<cursiv::AstNode> {
  string_view key;
  string_view rawValue; // interpret according to the key, not the lexical shape
  Locale *locale = nullptr;
};

struct Group : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Entry *> entries;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Group *> groups;
};

} // namespace desktopentry::ast
