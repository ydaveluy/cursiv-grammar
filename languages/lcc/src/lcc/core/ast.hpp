// The lcc language: its AST. A Library of Congress call number --
// a topic letter, an optional subtopic letter, dotted subclass numbers,
// cutter numbers and an optional date.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace lcc::ast {

struct Cutter : cursiv::Extends<cursiv::AstNode> {
  string_view letter;
  string_view digits;
};

struct CallNumber : cursiv::Extends<cursiv::AstNode> {
  string_view topic;
  optional<string_view> subtopic;
  vector<string_view> subclasses;
  vector<Cutter*> cutters;
  optional<string_view> date;
  optional<string_view> item;
};

}  // namespace lcc::ast
