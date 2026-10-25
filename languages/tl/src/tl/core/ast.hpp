#pragma once

#include <cstdint>
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tl::ast {

struct Proposition : cursiv::Extends<cursiv::AstNode> {};
struct Missing : cursiv::Extends<Proposition> {};
struct Bottom : cursiv::Extends<Proposition> {};
struct AtomicProp : cursiv::Extends<Proposition> { string_view name; };
struct Group : cursiv::Extends<Proposition> { Proposition* inner; };
struct Not : cursiv::Extends<Proposition> { Proposition* operand; };

enum class Tense : std::uint8_t { always, was };
struct TenseOp : cursiv::Extends<Proposition> {
  Tense tense;
  Proposition* operand;
};

struct Or : cursiv::Extends<Proposition> {
  Proposition* left;
  string_view op;
  Proposition* right;
};

struct Formula : cursiv::Extends<cursiv::AstNode> { Proposition* value; };

}  // namespace tl::ast
