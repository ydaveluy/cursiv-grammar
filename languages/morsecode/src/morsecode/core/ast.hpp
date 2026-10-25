// The morsecode language: its AST. One document, one node: a run of decoded
// characters, each read from a fixed dot/dash pattern.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace morsecode::ast {

struct Message : cursiv::Extends<cursiv::AstNode> {
  // ASCII and libcw's legacy byte codes for accented characters.
  vector<uint8> letters;
};

}  // namespace morsecode::ast
