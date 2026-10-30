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

  struct ADD : terminal<as<ast::Operator::add>(lit<"+">)> {};
  struct SUBTRACT : terminal<as<ast::Operator::subtract>(lit<"-">)> {};

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

  struct Notation : infix<Operand, ast::BinaryNotation,
                          levels(left(as<ast::Operator::multiply>(lit<"*">) | as<ast::Operator::divide>(lit<"/">)),
                                 left(ref<ADD> | ref<SUBTRACT>))> {};

  using entry = Notation;
  using ignored = WS;
};

} // namespace dice
