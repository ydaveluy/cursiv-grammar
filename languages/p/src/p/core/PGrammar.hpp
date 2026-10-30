// The P'' language: its grammar -- two atoms and one recursive grouping.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "p/core/ast.hpp"

namespace p {

struct P : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct R : terminal<as<ast::AtomKind::r>(lit<"R">)> {};

  struct Atom : rule<assign<&ast::Atom::kind>(ref<R> | as<ast::AtomKind::l>(lit<"λ">))> {};

  struct Iterate;
  static constexpr auto symbol = ref<Iterate> | ref<Atom>;

  struct Iterate
      : rule<lit<"("> + some(append<&ast::Iterate::symbols>(symbol)) +
             lit<")">> {};

  struct Prog : rule<some(append<&ast::Prog::symbols>(symbol))> {};

  using entry = Prog;
  using ignored = WS;
};

}  // namespace p
