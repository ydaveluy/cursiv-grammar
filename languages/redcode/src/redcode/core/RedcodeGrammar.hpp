// The redcode language: its grammar. ICWS94-flavoured Redcode, numeric
// addressing only: no labels, no identifiers.
#pragma once

#include <memory_resource>
#include <span>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "redcode/core/ast.hpp"

namespace redcode {

inline constexpr auto digits = [](std::string_view text, std::pmr::memory_resource *mem) {
  if (text.find_first_of(" \t") == std::string_view::npos) return text;
  const std::span<char> room = cursiv::text_room(mem, text.size());
  std::size_t n = 0;
  for (const char c : text) if (c >= '0' && c <= '9') room[n++] = c;
  return std::string_view(room.data(), n);
};

struct Redcode : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<some(cr<R"(\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};

  struct NUMBER : terminal<as<digits>(some(d + many(cr<R"( \t)">)))> {};

  struct DAT : terminal<as<ast::Opcode::dat>(ikw<"DAT">)> {};
  struct MOV : terminal<as<ast::Opcode::mov>(ikw<"MOV">)> {};
  struct ADD : terminal<as<ast::Opcode::add>(ikw<"ADD">)> {};
  struct SUB : terminal<as<ast::Opcode::sub>(ikw<"SUB">)> {};
  struct MUL : terminal<as<ast::Opcode::mul>(ikw<"MUL">)> {};
  struct DIV : terminal<as<ast::Opcode::div>(ikw<"DIV">)> {};
  struct MOD : terminal<as<ast::Opcode::mod>(ikw<"MOD">)> {};
  struct JMP : terminal<as<ast::Opcode::jmp>(ikw<"JMP">)> {};
  struct JMZ : terminal<as<ast::Opcode::jmz>(ikw<"JMZ">)> {};
  struct JMN : terminal<as<ast::Opcode::jmn>(ikw<"JMN">)> {};
  struct DJN : terminal<as<ast::Opcode::djn>(ikw<"DJN">)> {};
  struct CMP : terminal<as<ast::Opcode::cmp>(ikw<"CMP">)> {};
  struct SLT : terminal<as<ast::Opcode::slt>(ikw<"SLT">)> {};
  struct SPL : terminal<as<ast::Opcode::spl>(ikw<"SPL">)> {};
  struct ORG : terminal<as<ast::Opcode::org>(ikw<"ORG">)> {};
  struct DJZ : terminal<as<ast::Opcode::djz>(ikw<"DJZ">)> {};
  struct SEQ : terminal<as<ast::Opcode::seq>(ikw<"SEQ">)> {};
  struct SNE : terminal<as<ast::Opcode::sne>(ikw<"SNE">)> {};
  struct NOP : terminal<as<ast::Opcode::nop>(ikw<"NOP">)> {};
  struct LDP : terminal<as<ast::Opcode::ldp>(ikw<"LDP">)> {};
  struct STP : terminal<as<ast::Opcode::stp>(ikw<"STP">)> {};
  struct END : terminal<as<ast::Opcode::end>(ikw<"END">)> {};
  static constexpr auto opcode =
      ref<DAT> | ref<MOV> | ref<ADD> | ref<SUB> | ref<MUL> | ref<DIV> |
      ref<MOD> | ref<JMP> | ref<JMZ> | ref<JMN> | ref<DJN> | ref<CMP> |
      ref<SLT> | ref<SPL> | ref<DJZ> | ref<SEQ> | ref<SNE> | ref<NOP> |
      ref<LDP> | ref<STP>;

  static constexpr auto modifier = as<ast::Modifier::ab>(ikw<"AB">) | as<ast::Modifier::ba>(ikw<"BA">) | as<ast::Modifier::a>(ikw<"A">) |
                                    as<ast::Modifier::b>(ikw<"B">) | as<ast::Modifier::f>(ikw<"F">) | as<ast::Modifier::x>(ikw<"X">) |
                                    as<ast::Modifier::i>(ikw<"I">);

  static constexpr auto mmode = as<ast::Mode::immediate>(lit<"#">) | as<ast::Mode::direct>(lit<"$">) | as<ast::Mode::indirect>(lit<"@">) |
                                 as<ast::Mode::predecrement>(lit<"<">) | as<ast::Mode::postincrement>(lit<">">) |
                                 as<ast::Mode::a_indirect>(lit<"*">) |
                                 as<ast::Mode::a_predecrement>(lit<"{">) |
                                 as<ast::Mode::a_postincrement>(lit<"}">);


  struct Term;

  struct Expr
      : infix<Term, ast::BinaryExpr,
              levels(left(as<ast::Op::mul>(lit<"*">) | as<ast::Op::div>(lit<"/">) | as<ast::Op::mod>(lit<"%">)),
                     left(as<ast::Op::add>(lit<"+">) | as<ast::Op::sub>(lit<"-">)),
                     left(as<ast::Op::eq>(lit<"==">) | as<ast::Op::ne>(lit<"!=">) |
                          as<ast::Op::lt>(lit<"<">) | as<ast::Op::le>(lit<"<=">) |
                          as<ast::Op::gt>(lit<">">) | as<ast::Op::ge>(lit<">=">)),
                     left(as<ast::Op::logical_and>(lit<"&&">)),
                     left(as<ast::Op::logical_or>(lit<"||">)))> {};

  struct Term
      : rule<create<ast::Paren>(lit<"("> +
                                assign<&ast::Paren::inner>(ref<Expr>) +
                                lit<")">) |
             create<ast::Number>(
                 option(flag<&ast::Number::negative>(lit<"-">) |
                        lit<"+">) +
                 assign<&ast::Number::magnitude>(ref<NUMBER>)) |
             create<ast::Unary>(assign<&ast::Unary::op>(as<ast::Op::add>(lit<"+">) |
                 as<ast::Op::sub>(lit<"-">) | as<ast::Op::logical_not>(lit<"!">)) +
                 assign<&ast::Unary::operand>(ref<Term>))> {};

  struct Field
      : rule<option(assign<&ast::Field::mode>(mmode)) +
             assign<&ast::Field::value>(ref<Expr>)> {};

  struct Comment : rule<assign<&ast::Comment::text>(ref<COMMENT>)> {};
  struct DirectiveField : rule<assign<&ast::Field::value>(ref<Expr>)> {};

  struct Instruction
      : rule<((assign<&ast::Instruction::opcode>(opcode) +
             option(lit<"."> +
                    assign<&ast::Instruction::modifier>(modifier)) +
             assign<&ast::Instruction::field_a>(ref<Field>) +
             option(lit<","> +
                    assign<&ast::Instruction::field_b>(ref<Field>))) |
             (assign<&ast::Instruction::opcode>(ref<ORG> | ref<END>) +
              option(assign<&ast::Instruction::field_a>(ref<DirectiveField>)))) +
             option(assign<&ast::Instruction::comment>(ref<Comment>))> {};

  struct File
      : rule<create<ast::File> + many(ref<EOL> |
             (append<&ast::File::lines>(ref<Instruction> | ref<Comment>) +
              (ref<EOL> | eof)))> {};

  using entry = File;
  using ignored = WS;
};

}  // namespace redcode
