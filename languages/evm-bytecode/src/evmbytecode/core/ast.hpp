// The evmbytecode language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace evmbytecode::ast {

// One EVM instruction, in whichever of the three shapes it takes.
struct Opcode : cursiv::Extends<cursiv::AstNode> {};

// A mnemonic with no immediate: STOP, ADD, DUP3, SWAP16, JUMPDEST...
struct Instr : cursiv::Extends<Opcode> {
  string_view mnemonic;
  string_view data;
};

// PUSH1..PUSH32: the mnemonic decides the immediate's byte width, and `data`
// keeps the `0x`-prefixed hex literal exactly as spelled.
struct Push : cursiv::Extends<Opcode> {
  string_view mnemonic;
  string_view data;
};

// A byte no mnemonic names: `'fe'(Unknown Opcode)`.
struct Unknown : cursiv::Extends<Opcode> {
  string_view data;  // the two hex digits, quotes stripped
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Opcode*> opcodes;
};

}  // namespace evmbytecode::ast
