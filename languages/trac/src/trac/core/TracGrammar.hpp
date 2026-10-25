// The trac language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "trac/core/ast.hpp"

namespace trac {

struct Trac : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  // Letters and spaces share one class: a name or arg text may carry
  // embedded spaces ("read string"), so this token is greedy across them.
  struct STRING : terminal<some(cr<"a-zA-Z ">)> {};

  struct ActiveRule;
  struct NeutralRule;
  struct MacroRule : rule<ref<ActiveRule> | ref<NeutralRule>> {};

  struct ArgRule
      : rule<assign<&ast::Arg::value>(ref<MacroRule>) |
             (lit<"("> + assign<&ast::Arg::value>(ref<MacroRule>) +
              lit<")">) |
             assign<&ast::Arg::value>(ref<STRING>)> {};

  struct ActiveRule
      : rule<lit<"#"> + lit<"("> + assign<&ast::Active::name>(ref<STRING>) +
             option(lit<","> + some(append<&ast::Active::args>(ref<ArgRule>),
                                    lit<",">)) +
             lit<")">> {};

  struct NeutralRule
      : rule<lit<"##"> + lit<"("> + assign<&ast::Neutral::name>(ref<STRING>) +
             option(lit<","> +
                    some(append<&ast::Neutral::args>(ref<ArgRule>),
                         lit<",">)) +
             lit<")">> {};

  struct ProgramRule
      : rule<many(append<&ast::Program::macros>(ref<MacroRule>))> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace trac
