// The guitartab language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "guitartab/core/ast.hpp"

namespace guitartab {

struct Guitartab : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct NOTE : terminal<cr<"A-Ga-g">> {};
  struct MARK : terminal<cr<"xo-">> {};

  // A segment is one or more marks; trivia may separate them (`x - o`), so
  // each mark is its own token rather than one run swallowed by the lexer.
  struct PositionRule
      : rule<some(append<&ast::Position::marks>(ref<MARK>))> {};

  struct StringRule
      : rule<assign<&ast::StringLine::note>(ref<NOTE>) +
             some(append<&ast::StringLine::positions>(ref<PositionRule>) +
                  lit<"|">)> {};

  struct TabRule
      : rule<create<ast::Tab> +
             some(append<&ast::Tab::strings>(ref<StringRule>))> {};

  using entry = TabRule;
  using ignore = WS;
};

}  // namespace guitartab
