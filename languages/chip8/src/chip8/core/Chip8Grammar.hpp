// The chip8 language: its grammar. One flat instruction set, uppercase
// mnemonics, hex operands with no prefix.
#pragma once

#include <string_view>

#include "chip8/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace chip8 {

inline constexpr auto hex_value = [](std::string_view t) {
  int v = 0;
  for (const char c : t) v = v * 16 + (c <= '9' ? c - '0' : c - 'A' + 10);
  return v;
};

inline constexpr auto register_value = [](std::string_view t) {
  return hex_value(t.substr(1));
};

struct Chip8 : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};

  static constexpr auto DigitChar = cr<"0-9A-F">;

  struct DIGIT : terminal<as<hex_value>(DigitChar)> {};
  struct REGISTER : terminal<as<register_value>(lit<"V"> + DigitChar)> {};
  struct ADDR : terminal<as<hex_value>(repeat<3>(DigitChar))> {};
  struct BYTE : terminal<as<hex_value>(repeat<2>(DigitChar))> {};

  // One mnemonic, one opcode: a plain terminal carries the Op directly.
  struct CLS : terminal<as<ast::Op::Cls>(kw<"CLS">)> {};
  struct RET : terminal<as<ast::Op::Ret>(kw<"RET">)> {};
  struct SYS : terminal<as<ast::Op::Sys>(kw<"SYS">)> {};
  struct CALL : terminal<as<ast::Op::Call>(kw<"CALL">)> {};
  struct SNE : terminal<as<ast::Op::SneVxByte>(kw<"SNE">)> {};
  struct OR : terminal<as<ast::Op::Or>(kw<"OR">)> {};
  struct AND : terminal<as<ast::Op::And>(kw<"AND">)> {};
  struct XOR : terminal<as<ast::Op::Xor>(kw<"XOR">)> {};
  struct SUB : terminal<as<ast::Op::Sub>(kw<"SUB">)> {};
  struct SHR : terminal<as<ast::Op::Shr>(kw<"SHR">)> {};
  struct SUBN : terminal<as<ast::Op::Subn>(kw<"SUBN">)> {};
  struct SHL : terminal<as<ast::Op::Shl>(kw<"SHL">)> {};
  struct RND : terminal<as<ast::Op::Rnd>(kw<"RND">)> {};
  struct DRW : terminal<as<ast::Op::Drw>(kw<"DRW">)> {};
  struct SKP : terminal<as<ast::Op::Skp>(kw<"SKP">)> {};
  struct SKNP : terminal<as<ast::Op::Sknp>(kw<"SKNP">)> {};

  // A mnemonic shared by several shapes: each shape wraps it in its own rule,
  // one Op per wrapper, the lexer still sees one token.

  // Shared mnemonics use a data rule to select the instruction variant.

  struct Cls : rule<assign<&ast::Instruction::op>(ref<CLS>)> {};
  struct Ret : rule<assign<&ast::Instruction::op>(ref<RET>)> {};
  struct Sys : rule<assign<&ast::Instruction::op>(ref<SYS>) +
                        assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct Jp : rule<assign<&ast::Instruction::op>(as<ast::Op::Jp>(kw<"JP">)) +
                       assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct Call : rule<assign<&ast::Instruction::op>(ref<CALL>) +
                         assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct Se : rule<assign<&ast::Instruction::op>(as<ast::Op::SeVxByte>(kw<"SE">)) +
                       assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                       lit<","> + assign<&ast::Instruction::imm>(ref<BYTE>)> {
  };
  struct Sne : rule<assign<&ast::Instruction::op>(ref<SNE>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> + assign<&ast::Instruction::imm>(ref<BYTE>)> {
  };
  struct SeReg
      : rule<assign<&ast::Instruction::op>(as<ast::Op::SeVxVy>(kw<"SE">)) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct Ld : rule<assign<&ast::Instruction::op>(as<ast::Op::LdVxByte>(kw<"LD">)) +
                       assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                       lit<","> + assign<&ast::Instruction::imm>(ref<BYTE>)> {
  };
  struct Add
      : rule<assign<&ast::Instruction::op>(as<ast::Op::AddVxByte>(kw<"ADD">)) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::imm>(ref<BYTE>)> {};
  struct LdReg
      : rule<assign<&ast::Instruction::op>(as<ast::Op::LdVxVy>(kw<"LD">)) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct Or : rule<assign<&ast::Instruction::op>(ref<OR>) +
                       assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                       lit<","> +
                       assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct And : rule<assign<&ast::Instruction::op>(ref<AND>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> +
                        assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct Xor : rule<assign<&ast::Instruction::op>(ref<XOR>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> +
                        assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct AddReg
      : rule<assign<&ast::Instruction::op>(as<ast::Op::AddVxVy>(kw<"ADD">)) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct Sub : rule<assign<&ast::Instruction::op>(ref<SUB>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> +
                        assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct Shr
      : rule<assign<&ast::Instruction::op>(ref<SHR>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        option(lit<","> + assign<&ast::Instruction::reg2>(ref<REGISTER>))> {};
  struct Subn : rule<assign<&ast::Instruction::op>(ref<SUBN>) +
                         assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                         lit<","> +
                         assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct Shl
      : rule<assign<&ast::Instruction::op>(ref<SHL>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        option(lit<","> + assign<&ast::Instruction::reg2>(ref<REGISTER>))> {};
  struct SneReg
      : rule<assign<&ast::Instruction::op>(as<ast::Op::SneVxVy>(kw<"SNE">)) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct LdI : rule<assign<&ast::Instruction::op>(as<ast::Op::LdIAddr>(kw<"LD">)) +
                        kw<"I"> + lit<","> +
                        assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct JpV0 : rule<assign<&ast::Instruction::op>(as<ast::Op::JpV0Addr>(kw<"JP">)) +
                         kw<"V0"> + lit<","> +
                         assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct Rnd : rule<assign<&ast::Instruction::op>(ref<RND>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> + assign<&ast::Instruction::imm>(ref<BYTE>)> {
  };
  struct Drw
      : rule<assign<&ast::Instruction::op>(ref<DRW>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::imm>(ref<DIGIT>)> {};
  struct Skp : rule<assign<&ast::Instruction::op>(ref<SKP>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct Sknp : rule<assign<&ast::Instruction::op>(ref<SKNP>) +
                         assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdVxDt : rule<assign<&ast::Instruction::op>(as<ast::Op::LdVxDt>(kw<"LD">)) +
                           assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                           lit<","> + kw<"DT">> {};
  struct LdVxK : rule<assign<&ast::Instruction::op>(as<ast::Op::LdVxK>(kw<"LD">)) +
                          assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                          lit<","> + kw<"K">> {};
  struct LdDtVx : rule<assign<&ast::Instruction::op>(as<ast::Op::LdDtVx>(kw<"LD">)) +
                           kw<"DT"> + lit<","> +
                           assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdStVx : rule<assign<&ast::Instruction::op>(as<ast::Op::LdStVx>(kw<"LD">)) +
                           kw<"ST"> + lit<","> +
                           assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct AddIVx : rule<assign<&ast::Instruction::op>(as<ast::Op::AddIVx>(kw<"ADD">)) +
                           kw<"I"> + lit<","> +
                           assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdFVx : rule<assign<&ast::Instruction::op>(as<ast::Op::LdFVx>(kw<"LD">)) +
                          kw<"F"> + lit<","> +
                          assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdBVx : rule<assign<&ast::Instruction::op>(as<ast::Op::LdBVx>(kw<"LD">)) +
                          kw<"B"> + lit<","> +
                          assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdMemVx
      : rule<assign<&ast::Instruction::op>(as<ast::Op::LdMemVx>(kw<"LD"> + lit<"[">)) + kw<"I"> +
             lit<"]"> + lit<","> +
             assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdVxMem
      : rule<assign<&ast::Instruction::op>(as<ast::Op::LdVxMem>(kw<"LD">)) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             lit<"["> + kw<"I"> + lit<"]">> {};

  static constexpr auto instruction =
      ref<Cls> | ref<Ret> | ref<Sys> | ref<Jp> |
      ref<Call> | ref<Se> | ref<Sne> | ref<SeReg> |
      ref<Ld> | ref<Add> | ref<LdReg> | ref<Or> |
      ref<And> | ref<Xor> | ref<AddReg> | ref<Sub> |
      ref<Shr> | ref<Subn> | ref<Shl> | ref<SneReg> |
      ref<LdI> | ref<JpV0> | ref<Rnd> | ref<Drw> |
      ref<Skp> | ref<Sknp> | ref<LdVxDt> | ref<LdVxK> |
      ref<LdDtVx> | ref<LdStVx> | ref<AddIVx> | ref<LdFVx> |
      ref<LdBVx> | ref<LdMemVx> | ref<LdVxMem>;

  struct Program
      : rule<many(append<&ast::Program::instructions>(instruction))> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace chip8
