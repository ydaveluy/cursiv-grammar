#pragma once

#include <cstdint>
#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace tnt::ast {
struct Term : cursiv::Extends<cursiv::AstNode> {};
struct NumberLiteral : cursiv::Extends<Term> {};
struct Variable : cursiv::Extends<Term> {
  string_view letter;
  uint64 primes = 0;
};
struct SuccessorExpression : cursiv::Extends<Term> {
  uint64 count = 0;
  Term* expression = nullptr;
};
enum class BinaryOp : std::uint8_t { add, multiply };
struct BinaryExpression : cursiv::Extends<Term> {
  Term* left = nullptr;
  BinaryOp op;
  Term* right = nullptr;
};
struct GroupedExpression : cursiv::Extends<Term> { Term* expression = nullptr; };

struct Formula : cursiv::Extends<cursiv::AstNode> {};
struct Equation : cursiv::Extends<Formula> {
  Term* left = nullptr;
  Term* right = nullptr;
};
struct Negation : cursiv::Extends<Formula> {
  uint64 count = 0;
  Formula* expression = nullptr;
};
enum class Quantifier : std::uint8_t { forevery, exists };
struct QuantifiedExpression : cursiv::Extends<Formula> {
  Quantifier quantifier;
  Variable* variable = nullptr;
  Formula* expression = nullptr;
};
enum class LogicalOp : std::uint8_t { conjunction, disjunction, implication };
struct LogicalExpression : cursiv::Extends<Formula> {
  Formula* left = nullptr;
  LogicalOp op;
  Formula* right = nullptr;
};
}  // namespace tnt::ast
