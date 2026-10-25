#pragma once

#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace recfile::ast {

enum class Join { initial, newline, splice };

struct ValuePart : cursiv::Extends<cursiv::AstNode> {
  Join join = Join::initial;
  string_view text;
};

struct Line : cursiv::Extends<cursiv::AstNode> {};

struct Field : cursiv::Extends<Line> {
  string_view indentation;
  string_view key;
  vector<ValuePart*> parts;
  boolean terminated = false;
};

struct Comment : cursiv::Extends<Line> {
  string_view indentation;
  vector<string_view> lines;
};

struct RawLine : cursiv::Extends<Line> {
  string_view text;
};

struct Record : cursiv::Extends<cursiv::AstNode> {
  vector<Line*> lines;
};

using Entry = std::variant<Record*, Comment*, RawLine*>;

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Entry> entries;
};

}  // namespace recfile::ast
