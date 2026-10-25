#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace rfc1035::ast {

struct Domain : cursiv::Extends<cursiv::AstNode> {
  // Original label spellings; a final empty label records the root dot.
  vector<string_view> labels;
};

}  // namespace rfc1035::ast
