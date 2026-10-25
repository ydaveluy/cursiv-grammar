// The redcode language: its grammar. ICWS94-flavoured Redcode, numeric
// addressing only: no labels, no identifiers.
#pragma once

#include <cstdint>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "redcode/core/ast.hpp"

namespace redcode {

struct Redcode : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  struct NUMBER : terminal<some(d), std::int32_t> {};

  struct DAT : terminal<kw<"DAT">, value<ast::Opcode::dat>> {};
  struct MOV : terminal<kw<"MOV">, value<ast::Opcode::mov>> {};
  struct ADD : terminal<kw<"ADD">, value<ast::Opcode::add>> {};
  struct SUB : terminal<kw<"SUB">, value<ast::Opcode::sub>> {};
  struct MUL : terminal<kw<"MUL">, value<ast::Opcode::mul>> {};
  struct DIV : terminal<kw<"DIV">, value<ast::Opcode::div>> {};
  struct MOD : terminal<kw<"MOD">, value<ast::Opcode::mod>> {};
  struct JMP : terminal<kw<"JMP">, value<ast::Opcode::jmp>> {};
  struct JMZ : terminal<kw<"JMZ">, value<ast::Opcode::jmz>> {};
  struct JMN : terminal<kw<"JMN">, value<ast::Opcode::jmn>> {};
  struct DJN : terminal<kw<"DJN">, value<ast::Opcode::djn>> {};
  struct CMP : terminal<kw<"CMP">, value<ast::Opcode::cmp>> {};
  struct SLT : terminal<kw<"SLT">, value<ast::Opcode::slt>> {};
  struct SPL : terminal<kw<"SPL">, value<ast::Opcode::spl>> {};
  struct ORG : terminal<kw<"ORG">, value<ast::Opcode::org>> {};
  struct DJZ : terminal<kw<"DJZ">, value<ast::Opcode::djz>> {};
  static constexpr auto opcode =
      ref<DAT> | ref<MOV> | ref<ADD> | ref<SUB> | ref<MUL> | ref<DIV> |
      ref<MOD> | ref<JMP> | ref<JMZ> | ref<JMN> | ref<DJN> | ref<CMP> |
      ref<SLT> | ref<SPL> | ref<ORG> | ref<DJZ>;

  struct MOD_AB : terminal<kw<"AB">, value<ast::Modifier::ab>> {};
  struct MOD_BA : terminal<kw<"BA">, value<ast::Modifier::ba>> {};
  struct MOD_A : terminal<kw<"A">, value<ast::Modifier::a>> {};
  struct MOD_B : terminal<kw<"B">, value<ast::Modifier::b>> {};
  struct MOD_F : terminal<kw<"F">, value<ast::Modifier::f>> {};
  struct MOD_X : terminal<kw<"X">, value<ast::Modifier::x>> {};
  struct MOD_I : terminal<kw<"I">, value<ast::Modifier::i>> {};
  static constexpr auto modifier = ref<MOD_AB> | ref<MOD_BA> | ref<MOD_A> |
                                    ref<MOD_B> | ref<MOD_F> | ref<MOD_X> |
                                    ref<MOD_I>;

  struct IMMEDIATE : terminal<lit<"#">, value<ast::Mode::immediate>> {};
  struct DIRECT : terminal<lit<"$">, value<ast::Mode::direct>> {};
  struct INDIRECT : terminal<lit<"@">, value<ast::Mode::indirect>> {};
  struct PREDEC : terminal<lit<"<">, value<ast::Mode::predecrement>> {};
  struct POSTINC : terminal<lit<">">, value<ast::Mode::postincrement>> {};
  static constexpr auto mmode = ref<IMMEDIATE> | ref<DIRECT> | ref<INDIRECT> |
                                 ref<PREDEC> | ref<POSTINC>;

  struct ADDOP : terminal<lit<"+">, value<ast::Op::add>> {};
  struct SUBOP : terminal<lit<"-">, value<ast::Op::sub>> {};
  struct MULOP : terminal<lit<"*">, value<ast::Op::mul>> {};
  struct DIVOP : terminal<lit<"/">, value<ast::Op::div>> {};
  struct MODOP : terminal<lit<"%">, value<ast::Op::mod>> {};

  struct Term;

  struct ExprRule
      : infix<Term, &ast::BinaryExpr::left, &ast::BinaryExpr::op,
              &ast::BinaryExpr::right,
              (left(ref<MULOP> | ref<DIVOP> | ref<MODOP>) >>
               left(ref<ADDOP> | ref<SUBOP>))> {};

  struct Term
      : rule<create<ast::Paren>(lit<"("> +
                                assign<&ast::Paren::inner>(ref<ExprRule>) +
                                lit<")">) |
             create<ast::Number>(
                 option(flag<&ast::Number::negative>(lit<"-">) |
                        lit<"+">) +
                 assign<&ast::Number::magnitude>(ref<NUMBER>))> {};

  struct FieldRule
      : rule<option(assign<&ast::Field::mode>(mmode)) +
             assign<&ast::Field::value>(ref<ExprRule>)> {};

  struct CommentRule : rule<assign<&ast::Comment::text>(ref<COMMENT>)> {};

  struct InstructionRule
      : rule<assign<&ast::Instruction::opcode>(opcode) +
             option(lit<"."> +
                    assign<&ast::Instruction::modifier>(modifier)) +
             assign<&ast::Instruction::field_a>(ref<FieldRule>) +
             option(lit<","> +
                    assign<&ast::Instruction::field_b>(ref<FieldRule>)) +
             option(assign<&ast::Instruction::comment>(ref<CommentRule>))> {};

  struct FileRule
      : rule<some(append<&ast::File::lines>(ref<InstructionRule> |
                                            ref<CommentRule>) +
                  ref<EOL>)> {};

  using entry = FileRule;
  using ignore = WS;
};

}  // namespace redcode
