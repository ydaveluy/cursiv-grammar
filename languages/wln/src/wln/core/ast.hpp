#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace wln::ast {

struct Group : cursiv::Extends<cursiv::AstNode> {
  string_view separator;
  vector<string_view> symbols;
};

struct Record : cursiv::Extends<cursiv::AstNode> {
  string_view prefix;
  vector<Group *> groups;
  string_view trailing;
  optional<string_view> title;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Record *> records;
};

} // namespace wln::ast
