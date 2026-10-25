#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace qif::ast {

struct Entry : cursiv::Extends<cursiv::AstNode> {};

struct Header : cursiv::Extends<Entry> {
  string_view text;
};

struct Field : cursiv::Extends<cursiv::AstNode> {
  string_view code;
  string_view text;
};

struct Record : cursiv::Extends<Entry> {
  vector<Field *> fields;
  bool terminated = false;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Entry *> entries;
};

} // namespace qif::ast
