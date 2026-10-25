#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pdn::ast {

enum class Outcome : std::uint8_t {
  Draw,
  WhiteWin,
  BlackWin,
  Unknown,
  WhiteWin2,
  Draw2,
  BlackWin2,
  DoubleForfeit
};

struct Tag : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  string_view value;
};

struct Item : cursiv::Extends<cursiv::AstNode> {};

struct Ply : cursiv::Extends<Item> {
  string_view number;
  string_view notation;
  string_view strength;
  bool capture = false;
};

struct Variation : cursiv::Extends<Item> {
  vector<Item *> items;
};

struct Comment : cursiv::Extends<Item> {
  string_view text;
};

struct Setup : cursiv::Extends<Item> {
  string_view text;
};

struct Nag : cursiv::Extends<Item> {
  string_view value;
};

struct Game : cursiv::Extends<cursiv::AstNode> {
  vector<Tag *> tags;
  vector<Item *> items;
  vector<Outcome> results;
};

struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Game *> games;
};

} // namespace pdn::ast
