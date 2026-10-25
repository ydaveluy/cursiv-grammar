// The guitartab language: its AST. A tab is a set of strings, each a note
// name followed by the segments between successive bar '|' separators; a
// segment is one or more marks, and trivia may separate the marks.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace guitartab::ast {

struct Position : cursiv::Extends<cursiv::AstNode> {
  vector<string_view> marks;  // each '-', 'x' or 'o'
};

struct StringLine : cursiv::Extends<cursiv::AstNode> {
  string_view note;  // one of A-G, a-g
  vector<Position*> positions;
};

struct Tab : cursiv::Extends<cursiv::AstNode> {
  vector<StringLine*> strings;
};

}  // namespace guitartab::ast
