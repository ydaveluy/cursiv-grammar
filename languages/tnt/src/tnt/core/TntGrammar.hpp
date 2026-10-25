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
  struct FOREVERY : terminal<lit<"A"> | lit<"∀">, value<ast::Quantifier::forevery>> {};
  struct EXISTS : terminal<lit<"E"> | lit<"∃">, value<ast::Quantifier::exists>> {};
  struct PLUS : terminal<lit<"+">, value<ast::BinaryOp::add>> {};
  struct TIMES : terminal<lit<"*"> | lit<"."> | lit<"⋅">, value<ast::BinaryOp::multiply>> {};
  struct AND : terminal<lit<"&"> | lit<"^"> | lit<"∧">, value<ast::LogicalOp::conjunction>> {};
  struct OR : terminal<lit<"|"> | lit<"V"> | lit<"v"> | lit<"∨">, value<ast::LogicalOp::disjunction>> {};
  struct IMPLIES : terminal<lit<"]"> | lit<"⊃"> | lit<"→">, value<ast::LogicalOp::implication>> {};
  struct SuccessorRun : rule<some(ref<SUCCESSOR>), cursiv::text_conv<count_successors>> {};
  struct PrimeRun : rule<some(ref<PRIME>), cursiv::text_conv<count_primes>> {};
  struct NegationRun : rule<some(ref<NEGATOR>), cursiv::text_conv<count_negations>> {};
  struct VariableRule : rule<assign<&ast::Variable::letter>(ref<LETTER>) +
      option(assign<&ast::Variable::primes>(ref<PrimeRun>))> {};

  struct TermRule;
  struct GroupRule : rule<lit<"("> + assign<&ast::GroupedExpression::expression>(ref<TermRule>) + lit<")">> {};
  struct ZeroRule : rule<create<ast::NumberLiteral> + lit<"0">> {};
  static constexpr auto coreTerm = ref<GroupRule> | ref<ZeroRule> | ref<VariableRule>;
  struct SuccessorRule : rule<assign<&ast::SuccessorExpression::count>(ref<SuccessorRun>) +
      assign<&ast::SuccessorExpression::expression>(coreTerm)> {};
  struct PrimaryTerm : rule<ref<SuccessorRule> | coreTerm> {};
  struct TermRule : infix<PrimaryTerm, &ast::BinaryExpression::left,
      &ast::BinaryExpression::op, &ast::BinaryExpression::right,
      (left(ref<TIMES>) >> left(ref<PLUS>))> {};

  struct FormulaRule;
  struct EquationRule : rule<assign<&ast::Equation::left>(ref<TermRule>) + lit<"="> +
      assign<&ast::Equation::right>(ref<TermRule>)> {};
  struct QuantifiedRule : rule<assign<&ast::QuantifiedExpression::quantifier>(ref<FOREVERY> | ref<EXISTS>) +
      assign<&ast::QuantifiedExpression::variable>(ref<VariableRule>) + lit<":"> +
      assign<&ast::QuantifiedExpression::expression>(ref<FormulaRule>)> {};
  struct CompoundRule : rule<lit<"<"> + assign<&ast::LogicalExpression::left>(ref<FormulaRule>) +
      assign<&ast::LogicalExpression::op>(ref<AND> | ref<OR> | ref<IMPLIES>) +
      assign<&ast::LogicalExpression::right>(ref<FormulaRule>) + lit<">">> {};
  static constexpr auto baseFormula = ref<QuantifiedRule> | ref<CompoundRule> | ref<EquationRule>;
  struct NegationRule : rule<assign<&ast::Negation::count>(ref<NegationRun>) +
      assign<&ast::Negation::expression>(baseFormula)> {};
  struct FormulaRule : rule<ref<NegationRule> | baseFormula> {};
  using entry = FormulaRule;
  using ignore = WS;
};
}  // namespace tnt
