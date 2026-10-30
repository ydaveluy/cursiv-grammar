// The ltl example language: its AST -- one class per proposition form, a
// shared empty base so every operator's operands agree on one type.
#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ltl::ast {

struct Proposition : cursiv::Extends<cursiv::AstNode> {};

enum class UnaryOp : std::uint8_t {
  negate,
  globally,
  finally,
  next,
};

enum class BinaryOp : std::uint8_t {
  land,
  lor,
  implies,
  until,
  weak,
  release,
  strong_release,
  equivalent,
};

struct BoolLit : cursiv::Extends<Proposition> {
  boolean value;
};

struct AtomicProp : cursiv::Extends<Proposition> {
  string_view name;
};

struct UnaryExpr : cursiv::Extends<Proposition> {
  UnaryOp op;
  Proposition *operand;
};

struct BinaryExpr : cursiv::Extends<Proposition> {
  Proposition *left;
  BinaryOp op;
  Proposition *right;
};

} // namespace ltl::ast
