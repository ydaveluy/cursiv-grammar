// The chip8 language: its AST. One flat instruction set (35 opcodes, no
// nesting), so one generic node carries every shape rather than one class
// each.
#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace chip8::ast {

enum class Op {
  Cls,
  Ret,
  Sys,
  Jp,
  Call,
  SeVxByte,
  SneVxByte,
  SeVxVy,
  LdVxByte,
  AddVxByte,
  LdVxVy,
  Or,
  And,
  Xor,
  AddVxVy,
  Sub,
  Shr,
  Subn,
  Shl,
  SneVxVy,
  LdIAddr,
  JpV0Addr,
  Rnd,
  Drw,
  Skp,
  Sknp,
  LdVxDt,
  LdVxK,
  LdDtVx,
  LdStVx,
  AddIVx,
  LdFVx,
  LdBVx,
  LdMemVx,
  LdVxMem,
};

// -1 marks an operand this op does not use; which of reg1/reg2/imm apply
// follows from op alone.
struct Instruction : cursiv::Extends<cursiv::AstNode> {
  Op op;
  int reg1 = -1;
  int reg2 = -1;
  int imm = -1;
};

struct Program : cursiv::Extends<cursiv::AstNode> {
  vector<Instruction*> instructions;
};

}  // namespace chip8::ast
