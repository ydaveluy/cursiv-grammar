#pragma once
#include "cursiv/core/syntax-tree/AstNode.hpp"
namespace redis::ast {
struct Argument : cursiv::Extends<cursiv::AstNode> {
  string_view raw;
};
struct Command : cursiv::Extends<cursiv::AstNode> {
  Argument *name = nullptr;
  vector<Argument *> arguments;
};
struct Document : cursiv::Extends<cursiv::AstNode> {
  vector<Command *> commands;
};
} // namespace redis::ast
