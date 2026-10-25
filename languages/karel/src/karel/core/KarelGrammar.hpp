// The karel language: its grammar.
#pragma once

#include <cstdint>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "karel/core/ast.hpp"

namespace karel {

inline constexpr auto to_instr = [](std::string_view t) -> ast::Instr {
  using enum ast::Instr;
  if (t == "MOVE") return move;
  if (t == "TURNLEFT") return turn_left;
  if (t == "PICKBEEPER") return pick_beeper;
  if (t == "PUTBEEPER") return put_beeper;
  return turn_off;
};
using InstrConv = cursiv::text_conv<to_instr>;

inline constexpr auto to_cond = [](std::string_view t) -> ast::Cond {
  using enum ast::Cond;
  if (t == "FRONT-IS-CLEAR") return front_clear;
  if (t == "FRONT-IS-BLOCKED") return front_blocked;
  if (t == "LEFT-IS-CLEAR") return left_clear;
  if (t == "LEFT-IS-BLOCKED") return left_blocked;
  if (t == "RIGHT-IS-CLEAR") return right_clear;
  if (t == "RIGHT-IS-BLOCKED") return right_blocked;
  if (t == "BACK-IS-CLEAR") return back_clear;
  if (t == "BACK-IS-BLOCKED") return back_blocked;
  if (t == "NEXT-TO-A-BEEPER") return next_to_beeper;
  if (t == "NOT-NEXT-TO-A-BEEPER") return not_next_to_beeper;
  if (t == "ANY-BEEPERS-IN-BEEPER-BAG") return any_beepers_in_bag;
  if (t == "NO-BEEPERS-IN-BEEPER-BAG") return no_beepers_in_bag;
  if (t == "FACING-NORTH") return facing_north;
  if (t == "NOT-FACING-NORTH") return not_facing_north;
  if (t == "FACING-SOUTH") return facing_south;
  if (t == "NOT-FACING-SOUTH") return not_facing_south;
  if (t == "FACING-EAST") return facing_east;
  if (t == "NOT-FACING-EAST") return not_facing_east;
  if (t == "FACING-WEST") return facing_west;
  return not_facing_west;
};
using CondConv = cursiv::text_conv<to_cond>;

struct Karel : cursiv::GrammarSpec {
  // Identifiers and keywords both run on hyphens ('BEGINNING-OF-PROGRAM',
  // 'FRONT-IS-CLEAR'), so the boundary a kw<> may not run into must include
  // it -- the default `w` does not.
  static constexpr auto word_boundary = cr<"A-Za-z0-9-">;

  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  // LETTER (LETTER | DIGIT)*, LETTER: [A-Za-z] | '-'.
  struct ID : terminal<cr<"A-Za-z-"> + many(cr<"A-Za-z0-9-">)> {};

  // number: DIGIT+ -- folded into a terminal, no separate parse node.
  struct NUMBER : terminal<some(d), std::int64_t> {};

  struct INSTR : terminal<kw<"MOVE"> | kw<"TURNLEFT"> | kw<"PICKBEEPER"> |
                              kw<"PUTBEEPER"> | kw<"TURNOFF">,
                          InstrConv> {};

  struct CONDITION
      : terminal<kw<"FRONT-IS-CLEAR"> | kw<"FRONT-IS-BLOCKED"> |
                     kw<"LEFT-IS-CLEAR"> | kw<"LEFT-IS-BLOCKED"> |
                     kw<"RIGHT-IS-CLEAR"> | kw<"RIGHT-IS-BLOCKED"> |
                     kw<"BACK-IS-CLEAR"> | kw<"BACK-IS-BLOCKED"> |
                     kw<"NEXT-TO-A-BEEPER"> | kw<"NOT-NEXT-TO-A-BEEPER"> |
                     kw<"ANY-BEEPERS-IN-BEEPER-BAG"> |
                     kw<"NO-BEEPERS-IN-BEEPER-BAG"> | kw<"FACING-NORTH"> |
                     kw<"NOT-FACING-NORTH"> | kw<"FACING-SOUTH"> |
                     kw<"NOT-FACING-SOUTH"> | kw<"FACING-EAST"> |
                     kw<"NOT-FACING-EAST"> | kw<"FACING-WEST"> |
                     kw<"NOT-FACING-WEST">,
                 CondConv> {};

  struct BlockRule;
  struct IterationRule;
  struct LoopRule;
  struct ConditionalRule;
  struct InstructionRule;

  static constexpr auto statement =
      ref<BlockRule> | ref<IterationRule> | ref<LoopRule> |
      ref<ConditionalRule> | ref<InstructionRule>;

  struct BlockRule
      : rule<create<ast::Block> + kw<"BEGIN"> +
             many(append<&ast::Block::statements>(statement)) + kw<"END">> {};

  struct IterationRule
      : rule<kw<"ITERATE"> + assign<&ast::Iteration::count>(ref<NUMBER>) +
             kw<"TIMES"> + assign<&ast::Iteration::body>(statement)> {};

  struct LoopRule
      : rule<kw<"WHILE"> + assign<&ast::Loop::condition>(ref<CONDITION>) +
             kw<"DO"> + assign<&ast::Loop::body>(statement)> {};

  struct ConditionalRule
      : rule<kw<"IF"> + assign<&ast::Conditional::condition>(ref<CONDITION>) +
             kw<"THEN"> + assign<&ast::Conditional::thenBranch>(statement) +
             option(kw<"ELSE"> +
                    assign<&ast::Conditional::elseBranch>(statement))> {};

  struct InstructionRule
      : rule<assign<&ast::Instruction::op>(ref<INSTR> | ref<ID>)> {};

  struct DefinitionRule
      : rule<create<ast::Definition> + kw<"DEFINE"> +
             assign<&ast::Definition::name>(ref<ID>) + kw<"AS"> +
             assign<&ast::Definition::body>(statement)> {};

  struct KarelRule
      : rule<create<ast::Program> + kw<"BEGINNING-OF-PROGRAM"> +
             many(append<&ast::Program::definitions>(ref<DefinitionRule>)) +
             kw<"BEGINNING-OF-EXECUTION"> +
             many(append<&ast::Program::statements>(statement)) +
             kw<"END-OF-EXECUTION"> + kw<"END-OF-PROGRAM">> {};

  using entry = KarelRule;
  using ignore = WS;
};

}  // namespace karel
