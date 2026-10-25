// The postalcode language: its AST. One node, split into the two groups
// Canada Post calls the forward sortation area and the local delivery unit.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace postalcode::ast {

struct PostalCode : cursiv::Extends<cursiv::AstNode> {
  string_view forwardSortationArea;  // 3 chars: letter digit letter
  string_view localDeliveryUnit;     // 3 chars: digit letter digit
};

}  // namespace postalcode::ast
