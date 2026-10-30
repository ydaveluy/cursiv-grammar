#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "tnt/core/ast.hpp"

namespace tnt {
inline constexpr auto count_successors = [](std::string_view t) {
  return static_cast<std::uint64_t>(std::count(t.begin(), t.end(), 'S'));
};
inline constexpr auto count_primes = [](std::string_view t) {
  return static_cast<std::uint64_t>(std::count(t.begin(), t.end(), '\'') +
      std::count(t.begin(), t.end(), '\xE2'));
};
inline constexpr auto count_negations = [](std::string_view t) {
  return static_cast<std::uint64_t>(std::count(t.begin(), t.end(), '~') +
      std::count(t.begin(), t.end(), '!'));
};
struct Tnt : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct SUCCESSOR : terminal<lit<"S">> {};
  struct LETTER : terminal<cr<"a-z">> {};
  struct PRIME : terminal<lit<"'"> | lit<"′">> {};
  struct NEGATOR : terminal<cr<"~!">> {};
  struct FOREVERY
      : terminal<as<ast::Quantifier::forevery>(lit<"A"> | lit<"∀">)> {};
  struct EXISTS : terminal<as<ast::Quantifier::exists>(lit<"E"> | lit<"∃">)> {};
  struct TIMES
      : terminal<as<ast::BinaryOp::multiply>(lit<"*"> | lit<"."> | lit<"⋅">)> {
  };
  struct AND : terminal<as<ast::LogicalOp::conjunction>(lit<"&"> | lit<"^"> |
                                                        lit<"∧">)> {};
  struct OR : terminal<as<ast::LogicalOp::disjunction>(lit<"|"> | lit<"V"> |
                                                       lit<"v"> | lit<"∨">)> {};
  struct IMPLIES : terminal<as<ast::LogicalOp::implication>(
                       lit<"]"> | lit<"⊃"> | lit<"→">)> {};
  struct SuccessorRun : rule<as<count_successors>(some(ref<SUCCESSOR>))> {};
  struct PrimeRun : rule<as<count_primes>(some(ref<PRIME>))> {};
  struct NegationRun : rule<as<count_negations>(some(ref<NEGATOR>))> {};
  struct Variable : rule<assign<&ast::Variable::letter>(ref<LETTER>) +
      option(assign<&ast::Variable::primes>(ref<PrimeRun>))> {};

  struct Term;
  struct Group : rule<lit<"("> + assign<&ast::GroupedExpression::expression>(ref<Term>) + lit<")">> {};
  struct Zero : rule<create<ast::NumberLiteral> + lit<"0">> {};
  static constexpr auto coreTerm = ref<Group> | ref<Zero> | ref<Variable>;
  struct Successor : rule<assign<&ast::SuccessorExpression::count>(ref<SuccessorRun>) +
      assign<&ast::SuccessorExpression::expression>(coreTerm)> {};
  struct PrimaryTerm : rule<ref<Successor> | coreTerm> {};
  struct Term : infix<PrimaryTerm, ast::BinaryExpression,
      levels(left(ref<TIMES>), left(as<ast::BinaryOp::add>(lit<"+">)))> {};

  struct Formula;
  struct Equation : rule<assign<&ast::Equation::left>(ref<Term>) + lit<"="> +
      assign<&ast::Equation::right>(ref<Term>)> {};
  struct Quantified : rule<assign<&ast::QuantifiedExpression::quantifier>(ref<FOREVERY> | ref<EXISTS>) +
      assign<&ast::QuantifiedExpression::variable>(ref<Variable>) + lit<":"> +
      assign<&ast::QuantifiedExpression::expression>(ref<Formula>)> {};
  struct Compound : rule<lit<"<"> + assign<&ast::LogicalExpression::left>(ref<Formula>) +
      assign<&ast::LogicalExpression::op>(ref<AND> | ref<OR> | ref<IMPLIES>) +
      assign<&ast::LogicalExpression::right>(ref<Formula>) + lit<">">> {};
  static constexpr auto baseFormula = ref<Quantified> | ref<Compound> | ref<Equation>;
  struct Negation : rule<assign<&ast::Negation::count>(ref<NegationRun>) +
      assign<&ast::Negation::expression>(baseFormula)> {};
  struct Formula : rule<ref<Negation> | baseFormula> {};
  using entry = Formula;
  using ignored = WS;
};
}  // namespace tnt
