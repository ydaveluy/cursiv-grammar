#pragma once

#include <cstdint>
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace ctl::ast {

struct Proposition : cursiv::Extends<cursiv::AstNode> {};
struct Missing : cursiv::Extends<Proposition> {};
struct Top : cursiv::Extends<Proposition> {};
struct Bottom : cursiv::Extends<Proposition> {};
struct AtomicProp : cursiv::Extends<Proposition> { string_view name; bool quoted; };
struct Grouped : cursiv::Extends<Proposition> { Proposition* inner; };
struct Not : cursiv::Extends<Proposition> { Proposition* operand; };

enum class BinaryOp : std::uint8_t { conjunction, disjunction, implication, biconditional };
struct BinaryProp : cursiv::Extends<Proposition> {
  Proposition* left;
  BinaryOp op;
  Proposition* right;
};

enum class PathQuantifier : std::uint8_t { inevitable, exists };
struct Quantified : cursiv::Extends<Proposition> {
  PathQuantifier quantifier;
  Proposition* path;
};

enum class TemporalModality : std::uint8_t { next, finally, globally };
struct UnaryTemporal : cursiv::Extends<Proposition> {
  TemporalModality modality;
  Proposition* operand;
};

enum class TemporalRelation : std::uint8_t { until, release };
struct BinaryTemporal : cursiv::Extends<Proposition> {
  Proposition* left;
  TemporalRelation relation;
  Proposition* right;
};

}  // namespace ctl::ast
