// The ttm language: its AST. A call is active (`#<...>`) or neutral
// (`##<...>`); an argument is a nested call or plain text, the `<...>`
// wrapping a nested call is pure grouping and keeps no node of its own.
#pragma once

#include <string_view>
#include <variant>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ttm::ast {

struct Call;

using Arg = std::variant<Call*, std::string_view>;

struct Call : cursiv::Extends<cursiv::AstNode> {
  string_view name;
  vector<Arg> args;
};

struct Active : cursiv::Extends<Call> {};
struct Neutral : cursiv::Extends<Call> {};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Call*> calls;
};

}  // namespace ttm::ast
