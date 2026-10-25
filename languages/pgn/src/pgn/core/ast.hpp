#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pgn::ast {

enum class Result : std::uint8_t { white_wins, black_wins, drawn, unknown };
struct Element : cursiv::Extends<cursiv::AstNode> {};
struct MoveNumber : cursiv::Extends<Element> {
  string_view number;
  string_view dots;
};
struct SanMove : cursiv::Extends<Element> {
  string_view san;
};
struct Nag : cursiv::Extends<Element> {
  string_view number;
};
struct Comment : cursiv::Extends<Element> {
  string_view raw;
};
struct UnexpectedClose : cursiv::Extends<Element> {};
struct Variation : cursiv::Extends<Element> {
  vector<Element*> elements;
  boolean closed;
};
struct TagPair : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};
struct Game : cursiv::Extends<cursiv::AstNode> {
  vector<TagPair*> tags;
  vector<Element*> elements;
  Result result = Result::unknown;
  boolean terminated;
};
struct Database : cursiv::Extends<cursiv::AstNode> {
  vector<Game*> games;
};

}  // namespace pgn::ast
