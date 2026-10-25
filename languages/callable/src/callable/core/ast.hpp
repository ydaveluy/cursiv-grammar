// The callable language: its AST. A program is a list of calls; a call's
// arguments are each a nested call or a string, so `Arg` is a variant and
// only calls -- never strings -- are arena nodes.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace callable::ast {

struct Call;

using Arg = std::variant<Call*, std::string_view>;

struct Call : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Arg> args;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Call*> lines;
};

}  // namespace callable::ast
