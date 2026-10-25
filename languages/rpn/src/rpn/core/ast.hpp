// The rpn language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace rpn::ast {

enum class Operator { pow, plus, minus, times, div };

// A run of leading +/- signs before the operand; flattened here (order does
// not change the value) into one node instead of a recursive chain.
struct SignedAtom : cursiv::Extends<cursiv::AstNode> {
  vector<Operator> signs;
  variant<double, string_view> value;
};

// One RPN program: an operand, then a run of operands and operators applied
// left to right against the stack.
struct Expression : cursiv::Extends<cursiv::AstNode> {
  vector<variant<SignedAtom*, Operator>> items;
};

}  // namespace rpn::ast
