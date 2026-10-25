#pragma once

#include <cstdint>

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace propcalc::ast {

struct Expr : cursiv::Extends<cursiv::AstNode> {};

struct VariableRef : cursiv::Extends<Expr> {
  string_view name;
  vector<string_view> extraNames;
};

struct Negation : cursiv::Extends<Expr> {
  Expr* operand;
};

struct Grouped : cursiv::Extends<Expr> {
  Expr* expression;
};

enum class Connective : std::uint8_t {
  conjunction, disjunction, implication, equivalence
};

struct Binary : cursiv::Extends<Expr> {
  Expr* left;
  Connective op;
  Expr* right;
};

struct Proposition : cursiv::Extends<cursiv::AstNode> {
  Expr* premises;
  Expr* conclusion;
};

}  // namespace propcalc::ast
