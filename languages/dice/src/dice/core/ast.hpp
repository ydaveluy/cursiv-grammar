#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace dice::ast {

struct Notation : cursiv::Extends<cursiv::AstNode> {};

enum class Operator : std::uint8_t { add, subtract, multiply, divide };

struct BinaryNotation : cursiv::Extends<Notation> {
  Notation *left;
  Operator op;
  Notation *right;
};

struct NumberLiteral : cursiv::Extends<Notation> {
  boolean negative;
  string_view value;
};

struct DiceRoll : cursiv::Extends<Notation> {
  boolean negative;
  optional<string_view> count;
  string_view sides;
};

} // namespace dice::ast
