#pragma once
#include "cursiv/syntax-tree/AstNode.hpp"
namespace gtin::ast {
struct Gtin : cursiv::Extends<cursiv::AstNode> {
  string_view text;
};
}  // namespace gtin::ast
