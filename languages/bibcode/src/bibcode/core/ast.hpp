#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace bibcode::ast {

struct Bibcode : cursiv::Extends<cursiv::AstNode> {
  string_view year;
  string_view publish;
  string_view volume;
  string_view section;
  string_view page;
  string_view author;
};

}  // namespace bibcode::ast
