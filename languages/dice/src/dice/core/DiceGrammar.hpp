// The dice notation language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "dice/core/ast.hpp"

namespace dice {

struct Dice : cursiv::GrammarSpec {
  // Whitespace separates dice and arithmetic tokens.
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct NUMBER : terminal<some(d)> {};
  struct DSEP : terminal<cr<"dD">> {};

  struct ADD : terminal<lit<"+">, value<ast::Operator::add>> {};
  struct SUBTRACT : terminal<lit<"-">, value<ast::Operator::subtract>> {};
  struct MULTIPLY : terminal<lit<"*">, value<ast::Operator::multiply>> {};
  struct DIVIDE : terminal<lit<"/">, value<ast::Operator::divide>> {};

  struct Notation;

  struct Operand
      : rule<create<ast::DiceRoll>(
                 option(ref<ADD> |
                        flag<&ast::DiceRoll::negative>(ref<SUBTRACT>)) +
                 option(assign<&ast::DiceRoll::count>(ref<NUMBER>)) +
                 ref<DSEP> + assign<&ast::DiceRoll::sides>(ref<NUMBER>)) |
             create<ast::NumberLiteral>(
                 option(ref<ADD> | flag<&ast::NumberLiteral::negative>(
                                       ref<SUBTRACT>)) +
                 assign<&ast::NumberLiteral::value>(ref<NUMBER>)) |
             (lit<"("> + ref<Notation> + lit<")">)> {};

  struct Notation : infix<Operand, &ast::BinaryNotation::left,
                          &ast::BinaryNotation::op, &ast::BinaryNotation::right,
                          (left(ref<MULTIPLY> | ref<DIVIDE>) >>
                           left(ref<ADD> | ref<SUBTRACT>))> {};

  using entry = Notation;
  using ignore = WS;
};

} // namespace dice
