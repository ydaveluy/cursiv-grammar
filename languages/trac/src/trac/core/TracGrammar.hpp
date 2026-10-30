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

  struct Active;
  struct Neutral;
  struct Macro : rule<ref<Active> | ref<Neutral>> {};

  struct Arg
      : rule<assign<&ast::Arg::value>(ref<Macro>) |
             (flag<&ast::Arg::quoted>(lit<"(">) + assign<&ast::Arg::value>(ref<Macro>) +
              lit<")">) |
             assign<&ast::Arg::value>(ref<STRING>)> {};

  struct Active
      : rule<lit<"#"> + lit<"("> + assign<&ast::Active::name>(ref<STRING>) +
             option(lit<","> + some(append<&ast::Active::args>(ref<Arg>),
                                    lit<",">)) +
             lit<")">> {};

  struct Neutral
      : rule<lit<"##"> + lit<"("> + assign<&ast::Neutral::name>(ref<STRING>) +
             option(lit<","> +
                    some(append<&ast::Neutral::args>(ref<Arg>),
                         lit<",">)) +
             lit<")">> {};

  struct Program
      : rule<many(append<&ast::Program::macros>(ref<Macro>))> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace trac
