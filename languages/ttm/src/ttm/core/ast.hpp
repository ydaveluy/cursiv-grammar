// Default-delimiter TTM source. Text keeps escapes and angle quoting verbatim.
#pragma once

#include <string_view>
#include <variant>
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ttm::ast {
struct Call;
struct Argument;
using Part = std::variant<Call *, std::string_view>;

struct Argument : cursiv::Extends<cursiv::AstNode> {
  vector<Part> parts;
};
struct Call : cursiv::Extends<cursiv::AstNode> {
  Argument *name = nullptr;
  vector<Argument *> args;
};
struct Active : cursiv::Extends<Call> {};
struct Neutral : cursiv::Extends<Call> {};
struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Part> parts;
};
} // namespace ttm::ast
