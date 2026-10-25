#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace sieve::ast {
struct Argument : cursiv::Extends<cursiv::AstNode> {};
struct String : cursiv::Extends<Argument> {
  string_view raw;
};
struct Number : cursiv::Extends<Argument> {
  string_view text;
};
struct Tag : cursiv::Extends<Argument> {
  string_view text;
};
struct StringList : cursiv::Extends<Argument> {
  vector<String *> strings;
};
struct Test;
struct Arguments : cursiv::Extends<cursiv::AstNode> {
  vector<Argument *> items;
  vector<Test *> tests;
  boolean test_list = false;
};
struct Test : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Arguments *arguments = nullptr;
};
struct Command : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  Arguments *arguments = nullptr;
  vector<Command *> body;
  boolean has_block = false;
};
struct Script : cursiv::Extends<cursiv::AstNode> {
  vector<Command *> commands;
};
} // namespace sieve::ast
