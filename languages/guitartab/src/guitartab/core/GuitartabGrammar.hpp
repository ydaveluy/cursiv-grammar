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
  struct Position
      : rule<some(append<&ast::Position::marks>(ref<MARK>))> {};

  struct String
      : rule<assign<&ast::StringLine::note>(ref<NOTE>) +
             some(append<&ast::StringLine::positions>(ref<Position>) +
                  lit<"|">)> {};

  struct Tab
      : rule<create<ast::Tab> +
             some(append<&ast::Tab::strings>(ref<String>))> {};

  using entry = Tab;
  using ignored = WS;
};

}  // namespace guitartab
