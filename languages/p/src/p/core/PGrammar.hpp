// The P'' language: its grammar -- two atoms and one recursive grouping.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "p/core/ast.hpp"

namespace p {

struct P : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct R : terminal<lit<"R">, value<ast::AtomKind::r>> {};
  struct L : terminal<lit<"λ">, value<ast::AtomKind::l>> {};

  struct AtomRule : rule<assign<&ast::Atom::kind>(ref<R> | ref<L>)> {};

  struct IterateRule;
  static constexpr auto symbol = ref<IterateRule> | ref<AtomRule>;

  struct IterateRule
      : rule<lit<"("> + some(append<&ast::Iterate::symbols>(symbol)) +
             lit<")">> {};

  struct ProgRule : rule<some(append<&ast::Prog::symbols>(symbol))> {};

  using entry = ProgRule;
  using ignore = WS;
};

}  // namespace p
