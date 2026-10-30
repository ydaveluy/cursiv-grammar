#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "propcalc/core/ast.hpp"

namespace propcalc {

struct Propcalc : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct VARIABLE : terminal<some(cr<"a-uw-zA-Z">)> {};
  struct AND : terminal<as<ast::Connective::conjunction>(lit<"^">)> {};

  struct Expression;

  struct Atom : rule<
      create<ast::Negation>(lit<"!"> + assign<&ast::Negation::operand>(ref<Atom>)) |
      create<ast::Grouped>(lit<"("> + assign<&ast::Grouped::expression>(ref<Expression>) + lit<")">) |
      create<ast::VariableRef>(assign<&ast::VariableRef::name>(ref<VARIABLE>) +
          many(append<&ast::VariableRef::extraNames>(ref<VARIABLE>))) |
      create<ast::VariableRef>> {};

  struct Expression
      : infix<Atom, ast::Binary,
              levels(left(ref<AND>),
                     left(as<ast::Connective::disjunction>(lit<"v">)),
                     left(as<ast::Connective::implication>(lit<"->">)),
                     left(as<ast::Connective::equivalence>(lit<"<->">)))> {};

  struct Proposition : rule<assign<&ast::Proposition::premises>(ref<Expression>) +
      lit<"|-"> + assign<&ast::Proposition::conclusion>(ref<Expression>)> {};

  using entry = Proposition;
  using ignored = WS;
};

}  // namespace propcalc
