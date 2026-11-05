#pragma once

#include "cursiv/syntax-tree/AstNode.hpp"

namespace cmake::ast {

struct Argument : cursiv::Extends<cursiv::AstNode> {};

struct UnquotedArgument : cursiv::Extends<Argument> {
  string text;
};

struct QuotedArgument : cursiv::Extends<Argument> {
  string text;
};

struct BracketArgument : cursiv::Extends<Argument> {
  string text;
};

struct CompoundArgument : cursiv::Extends<Argument> {
  vector<Argument*> arguments;
};

struct CommandInvocation : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Argument*> arguments;
};

struct File : cursiv::Extends<cursiv::AstNode> {
  vector<CommandInvocation*> commands;
};

}  // namespace cmake::ast
