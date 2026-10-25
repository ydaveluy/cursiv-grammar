#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "propcalc/core/ast.hpp"

namespace propcalc {

struct Propcalc : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct VARIABLE : terminal<some(cr<"a-uw-zA-Z">)> {};
  struct AND : terminal<lit<"^">, value<ast::Connective::conjunction>> {};
  struct OR : terminal<lit<"v">, value<ast::Connective::disjunction>> {};
  struct IMPLIES : terminal<lit<"->">, value<ast::Connective::implication>> {};
  struct EQUIV : terminal<lit<"<->">, value<ast::Connective::equivalence>> {};

  struct Expression;

  struct Atom : rule<
      create<ast::Negation>(lit<"!"> + assign<&ast::Negation::operand>(ref<Atom>)) |
      create<ast::Grouped>(lit<"("> + assign<&ast::Grouped::expression>(ref<Expression>) + lit<")">) |
      create<ast::VariableRef>(assign<&ast::VariableRef::name>(ref<VARIABLE>) +
          many(append<&ast::VariableRef::extraNames>(ref<VARIABLE>))) |
      create<ast::VariableRef>> {};

  struct Expression : infix<Atom, &ast::Binary::left, &ast::Binary::op,
      &ast::Binary::right, (left(ref<AND>) >> left(ref<OR>) >>
                           left(ref<IMPLIES>) >> left(ref<EQUIV>))> {};

  struct PropositionRule : rule<assign<&ast::Proposition::premises>(ref<Expression>) +
      lit<"|-"> + assign<&ast::Proposition::conclusion>(ref<Expression>)> {};

  using entry = PropositionRule;
  using ignore = WS;
};

}  // namespace propcalc
