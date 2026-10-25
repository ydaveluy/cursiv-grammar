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
using HexValue = cursiv::text_conv<hex_value>;

inline constexpr auto register_value = [](std::string_view t) {
  return hex_value(t.substr(1));
};
using RegisterValue = cursiv::text_conv<register_value>;

struct Chip8 : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \r\n\t)">)> {};

  static constexpr auto DigitChar = cr<"0-9A-F">;

  struct DIGIT : terminal<DigitChar, HexValue> {};
  struct REGISTER : terminal<lit<"V"> + DigitChar, RegisterValue> {};
  struct ADDR : terminal<repeat<3>(DigitChar), HexValue> {};
  struct BYTE : terminal<repeat<2>(DigitChar), HexValue> {};

  // One mnemonic, one opcode: a plain terminal carries the Op directly.
  struct CLS : terminal<kw<"CLS">, value<ast::Op::Cls>> {};
  struct RET : terminal<kw<"RET">, value<ast::Op::Ret>> {};
  struct SYS : terminal<kw<"SYS">, value<ast::Op::Sys>> {};
  struct CALL : terminal<kw<"CALL">, value<ast::Op::Call>> {};
  struct SNE : terminal<kw<"SNE">, value<ast::Op::SneVxByte>> {};
  struct OR : terminal<kw<"OR">, value<ast::Op::Or>> {};
  struct AND : terminal<kw<"AND">, value<ast::Op::And>> {};
  struct XOR : terminal<kw<"XOR">, value<ast::Op::Xor>> {};
  struct SUB : terminal<kw<"SUB">, value<ast::Op::Sub>> {};
  struct SHR : terminal<kw<"SHR">, value<ast::Op::Shr>> {};
  struct SUBN : terminal<kw<"SUBN">, value<ast::Op::Subn>> {};
  struct SHL : terminal<kw<"SHL">, value<ast::Op::Shl>> {};
  struct RND : terminal<kw<"RND">, value<ast::Op::Rnd>> {};
  struct DRW : terminal<kw<"DRW">, value<ast::Op::Drw>> {};
  struct SKP : terminal<kw<"SKP">, value<ast::Op::Skp>> {};
  struct SKNP : terminal<kw<"SKNP">, value<ast::Op::Sknp>> {};

  // A mnemonic shared by several shapes: each shape wraps it in its own rule,
  // one Op per wrapper, the lexer still sees one token.
  struct JpOp : rule<kw<"JP">, value<ast::Op::Jp>> {};
  struct JpV0Op : rule<kw<"JP">, value<ast::Op::JpV0Addr>> {};
  struct SeByteOp : rule<kw<"SE">, value<ast::Op::SeVxByte>> {};
  struct SeRegOp : rule<kw<"SE">, value<ast::Op::SeVxVy>> {};
  struct AddByteOp : rule<kw<"ADD">, value<ast::Op::AddVxByte>> {};
  struct AddRegOp : rule<kw<"ADD">, value<ast::Op::AddVxVy>> {};
  struct AddIOp : rule<kw<"ADD">, value<ast::Op::AddIVx>> {};
  struct LdByteOp : rule<kw<"LD">, value<ast::Op::LdVxByte>> {};
  struct LdRegOp : rule<kw<"LD">, value<ast::Op::LdVxVy>> {};
  struct LdIOp : rule<kw<"LD">, value<ast::Op::LdIAddr>> {};
  struct LdVxDtOp : rule<kw<"LD">, value<ast::Op::LdVxDt>> {};
  struct LdVxKOp : rule<kw<"LD">, value<ast::Op::LdVxK>> {};
  struct LdDtVxOp : rule<kw<"LD">, value<ast::Op::LdDtVx>> {};
  struct LdStVxOp : rule<kw<"LD">, value<ast::Op::LdStVx>> {};
  struct LdFVxOp : rule<kw<"LD">, value<ast::Op::LdFVx>> {};
  struct LdBVxOp : rule<kw<"LD">, value<ast::Op::LdBVx>> {};
  struct LdVxMemOp : rule<kw<"LD">, value<ast::Op::LdVxMem>> {};

  // Shared mnemonics use a data rule to select the instruction variant.
  struct SneRegOp : rule<kw<"SNE">, value<ast::Op::SneVxVy>> {};
  struct LdMemVxOp : rule<kw<"LD"> + lit<"[">, value<ast::Op::LdMemVx>> {};

  struct ClsRule : rule<assign<&ast::Instruction::op>(ref<CLS>)> {};
  struct RetRule : rule<assign<&ast::Instruction::op>(ref<RET>)> {};
  struct SysRule : rule<assign<&ast::Instruction::op>(ref<SYS>) +
                        assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct JpRule : rule<assign<&ast::Instruction::op>(ref<JpOp>) +
                       assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct CallRule : rule<assign<&ast::Instruction::op>(ref<CALL>) +
                         assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct SeRule : rule<assign<&ast::Instruction::op>(ref<SeByteOp>) +
                       assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                       lit<","> + assign<&ast::Instruction::imm>(ref<BYTE>)> {
  };
  struct SneRule : rule<assign<&ast::Instruction::op>(ref<SNE>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> + assign<&ast::Instruction::imm>(ref<BYTE>)> {
  };
  struct SeRegRule
      : rule<assign<&ast::Instruction::op>(ref<SeRegOp>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct LdRule : rule<assign<&ast::Instruction::op>(ref<LdByteOp>) +
                       assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                       lit<","> + assign<&ast::Instruction::imm>(ref<BYTE>)> {
  };
  struct AddRule
      : rule<assign<&ast::Instruction::op>(ref<AddByteOp>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::imm>(ref<BYTE>)> {};
  struct LdRegRule
      : rule<assign<&ast::Instruction::op>(ref<LdRegOp>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct OrRule : rule<assign<&ast::Instruction::op>(ref<OR>) +
                       assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                       lit<","> +
                       assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct AndRule : rule<assign<&ast::Instruction::op>(ref<AND>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> +
                        assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct XorRule : rule<assign<&ast::Instruction::op>(ref<XOR>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> +
                        assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct AddRegRule
      : rule<assign<&ast::Instruction::op>(ref<AddRegOp>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct SubRule : rule<assign<&ast::Instruction::op>(ref<SUB>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> +
                        assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct ShrRule
      : rule<assign<&ast::Instruction::op>(ref<SHR>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        option(lit<","> + assign<&ast::Instruction::reg2>(ref<REGISTER>))> {};
  struct SubnRule : rule<assign<&ast::Instruction::op>(ref<SUBN>) +
                         assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                         lit<","> +
                         assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct ShlRule
      : rule<assign<&ast::Instruction::op>(ref<SHL>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        option(lit<","> + assign<&ast::Instruction::reg2>(ref<REGISTER>))> {};
  struct SneRegRule
      : rule<assign<&ast::Instruction::op>(ref<SneRegOp>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>)> {};
  struct LdIRule : rule<assign<&ast::Instruction::op>(ref<LdIOp>) +
                        kw<"I"> + lit<","> +
                        assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct JpV0Rule : rule<assign<&ast::Instruction::op>(ref<JpV0Op>) +
                         lit<"V"> + lit<"0"> + lit<","> +
                         assign<&ast::Instruction::imm>(ref<ADDR>)> {};
  struct RndRule : rule<assign<&ast::Instruction::op>(ref<RND>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                        lit<","> + assign<&ast::Instruction::imm>(ref<BYTE>)> {
  };
  struct DrwRule
      : rule<assign<&ast::Instruction::op>(ref<DRW>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::reg2>(ref<REGISTER>) + lit<","> +
             assign<&ast::Instruction::imm>(ref<DIGIT>)> {};
  struct SkpRule : rule<assign<&ast::Instruction::op>(ref<SKP>) +
                        assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct SknpRule : rule<assign<&ast::Instruction::op>(ref<SKNP>) +
                         assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdVxDtRule : rule<assign<&ast::Instruction::op>(ref<LdVxDtOp>) +
                           assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                           lit<","> + kw<"DT">> {};
  struct LdVxKRule : rule<assign<&ast::Instruction::op>(ref<LdVxKOp>) +
                          assign<&ast::Instruction::reg1>(ref<REGISTER>) +
                          lit<","> + kw<"K">> {};
  struct LdDtVxRule : rule<assign<&ast::Instruction::op>(ref<LdDtVxOp>) +
                           kw<"DT"> + lit<","> +
                           assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdStVxRule : rule<assign<&ast::Instruction::op>(ref<LdStVxOp>) +
                           kw<"ST"> + lit<","> +
                           assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct AddIVxRule : rule<assign<&ast::Instruction::op>(ref<AddIOp>) +
                           kw<"I"> + lit<","> +
                           assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdFVxRule : rule<assign<&ast::Instruction::op>(ref<LdFVxOp>) +
                          kw<"F"> + lit<","> +
                          assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdBVxRule : rule<assign<&ast::Instruction::op>(ref<LdBVxOp>) +
                          kw<"B"> + lit<","> +
                          assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdMemVxRule
      : rule<assign<&ast::Instruction::op>(ref<LdMemVxOp>) + kw<"I"> +
             lit<"]"> + lit<","> +
             assign<&ast::Instruction::reg1>(ref<REGISTER>)> {};
  struct LdVxMemRule
      : rule<assign<&ast::Instruction::op>(ref<LdVxMemOp>) +
             assign<&ast::Instruction::reg1>(ref<REGISTER>) + lit<","> +
             lit<"["> + kw<"I"> + lit<"]">> {};

  static constexpr auto instruction =
      ref<ClsRule> | ref<RetRule> | ref<SysRule> | ref<JpRule> |
      ref<CallRule> | ref<SeRule> | ref<SneRule> | ref<SeRegRule> |
      ref<LdRule> | ref<AddRule> | ref<LdRegRule> | ref<OrRule> |
      ref<AndRule> | ref<XorRule> | ref<AddRegRule> | ref<SubRule> |
      ref<ShrRule> | ref<SubnRule> | ref<ShlRule> | ref<SneRegRule> |
      ref<LdIRule> | ref<JpV0Rule> | ref<RndRule> | ref<DrwRule> |
      ref<SkpRule> | ref<SknpRule> | ref<LdVxDtRule> | ref<LdVxKRule> |
      ref<LdDtVxRule> | ref<LdStVxRule> | ref<AddIVxRule> | ref<LdFVxRule> |
      ref<LdBVxRule> | ref<LdMemVxRule> | ref<LdVxMemRule>;

  struct ProgramRule
      : rule<many(append<&ast::Program::instructions>(instruction))> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace chip8
