#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sexpression::ast {
struct Discard;
struct Item : cursiv::Extends<cursiv::AstNode> {
  vector<Discard *> discards;
};
struct Atom : cursiv::Extends<Item> {
  string_view text;
};
struct String : cursiv::Extends<Item> {
  string_view text;
};
struct Character : cursiv::Extends<Item> {
  string_view text;
};
struct Boolean : cursiv::Extends<Item> {
  string_view text;
};
struct Keyword : cursiv::Extends<Item> {
  string_view text;
};
struct Bytes : cursiv::Extends<Item> {
  string_view text;
};
struct Regex : cursiv::Extends<Item> {
  string_view text;
};
struct GraphRef : cursiv::Extends<Item> {
  string_view text;
};
struct List : cursiv::Extends<Item> {
  string_view opener;
  vector<Item *> items;
  Item *tail;
  boolean infix;
  vector<Item *> suffix;
  string_view closer;
};
struct Prefix : cursiv::Extends<Item> {
  string_view marker;
  Item *value;
};
struct Discard : cursiv::Extends<Item> {
  Item *value;
};
struct Directive : cursiv::Extends<Item> {
  string_view text;
};
struct Sexpr : cursiv::Extends<cursiv::AstNode> {
  vector<Item *> items;
};
} // namespace sexpression::ast
