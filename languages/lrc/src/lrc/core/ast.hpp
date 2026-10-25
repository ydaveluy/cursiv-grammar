#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace lrc::ast {

struct Timestamp : cursiv::Extends<cursiv::AstNode> {
  string_view minutes;
  string_view seconds;
  optional<string_view> fraction;
};

struct Row : cursiv::Extends<cursiv::AstNode> {};

struct Line : cursiv::Extends<Row> {
  vector<Timestamp*> timestamps;
  string_view text;
};

struct Metadata : cursiv::Extends<Row> {
  string_view key;
  string_view value;
  string_view tail;
};

struct RawLine : cursiv::Extends<Row> {
  string_view text;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Row*> lines;
};

}  // namespace lrc::ast
