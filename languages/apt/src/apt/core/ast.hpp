// The apt language: its AST. One line of an apt sources list -- a `deb` or
// `deb-src` record, optional key=value options, a URI, a distribution and
// its components.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace apt::ast {

enum class Type : std::uint8_t { Deb, DebSrc };

struct Option : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

struct Record : cursiv::Extends<cursiv::AstNode> {
  boolean commented;
  Type type;
  vector<Option*> options;
  string_view uri;
  string_view distribution;
  vector<string_view> components;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Record *> records;
};

}  // namespace apt::ast
