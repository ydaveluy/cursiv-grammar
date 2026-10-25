// The pmmn language: its AST. Plain structs -- the grammar names
// their members, and the generated builder fills them.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace pmmn::ast {

struct Command : cursiv::Extends<cursiv::AstNode> {};

// Also a Command: `dec(n);` alone is the bare test statement, and the same
// node is the condition read by If and While.
struct Test : cursiv::Extends<Command> {
  uint64 counter;
};

struct Inc : cursiv::Extends<Command> {
  uint64 counter;
};

struct Block : cursiv::Extends<cursiv::AstNode> {
  vector<Command*> commands;
};

struct If : cursiv::Extends<Command> {
  Test* test;
  Block* thenBranch;
  Block* elseBranch;
};

struct While : cursiv::Extends<Command> {
  Test* test;
  Block* body;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Command*> commands;
};

}  // namespace pmmn::ast
