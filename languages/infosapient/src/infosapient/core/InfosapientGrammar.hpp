// The infosapient language: its grammar.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "infosapient/core/ast.hpp"

namespace infosapient {

struct Infosapient : cursiv::GrammarSpec {
  // Rule words are space-separated (`if foo is extremely 5.0;`).
  struct WS : terminal<some(s)> {};

  struct IDENTIFIER : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9">)> {};
  struct FP_LITERAL : terminal<many(d) + lit<"."> + many(d), double> {};

  static constexpr auto hedge_conv = [](std::string_view t) {
    if (t == "about")
      return ast::Hedge::about;
    if (t == "above")
      return ast::Hedge::above;
    if (t == "after")
      return ast::Hedge::after;
    if (t == "around")
      return ast::Hedge::around;
    if (t == "before")
      return ast::Hedge::before;
    if (t == "below")
      return ast::Hedge::below;
    if (t == "closeTo")
      return ast::Hedge::close_to;
    if (t == "definitely")
      return ast::Hedge::definitely;
    if (t == "extremely")
      return ast::Hedge::extremely;
    if (t == "generally")
      return ast::Hedge::generally;
    if (t == "mostly")
      return ast::Hedge::mostly;
    if (t == "must")
      return ast::Hedge::must;
    if (t == "near")
      return ast::Hedge::near;
    if (t == "negative")
      return ast::Hedge::negative;
    if (t == "negatively")
      return ast::Hedge::negatively;
    if (t == "not")
      return ast::Hedge::not_;
    if (t == "positive")
      return ast::Hedge::positive;
    if (t == "positively")
      return ast::Hedge::positively;
    if (t == "roughly")
      return ast::Hedge::roughly;
    if (t == "should")
      return ast::Hedge::should;
    if (t == "slightly")
      return ast::Hedge::slightly;
    if (t == "somewhat")
      return ast::Hedge::somewhat;
    if (t == "very")
      return ast::Hedge::very;
    return ast::Hedge::in_vicinity_of;
  };
  struct HEDGE
      : terminal<kw<"about"> | kw<"above"> | kw<"after"> | kw<"around"> |
                     kw<"before"> | kw<"below"> | kw<"closeTo"> |
                     kw<"definitely"> | kw<"extremely"> | kw<"generally"> |
                     kw<"mostly"> | kw<"must"> | kw<"near"> | kw<"negative"> |
                     kw<"negatively"> | kw<"not"> | kw<"positive"> |
                     kw<"positively"> | kw<"roughly"> | kw<"should"> |
                     kw<"slightly"> | kw<"somewhat"> | kw<"very"> |
                     kw<"inVicinityOf">,
                 cursiv::text_conv<hedge_conv>> {};

  static constexpr auto restriction_conv = [](std::string_view t) {
    return t == "increased" ? ast::Restriction::increased
                            : ast::Restriction::decreased;
  };
  struct RESTRICTION : terminal<kw<"increased"> | kw<"decreased">,
                                cursiv::text_conv<restriction_conv>> {};

  static constexpr auto operator_conv = [](std::string_view t) {
    if (t == "and")
      return ast::LogicalOperator::and_;
    if (t == "boundedAnd")
      return ast::LogicalOperator::bounded_and;
    if (t == "cosineNot")
      return ast::LogicalOperator::cosine_not;
    if (t == "meanAnd")
      return ast::LogicalOperator::mean_and;
    if (t == "meanOr")
      return ast::LogicalOperator::mean_or;
    if (t == "or")
      return ast::LogicalOperator::or_;
    if (t == "productAnd")
      return ast::LogicalOperator::product_and;
    if (t == "productOr")
      return ast::LogicalOperator::product_or;
    if (t == "sugenoNot")
      return ast::LogicalOperator::sugeno_not;
    if (t == "thresholdNot")
      return ast::LogicalOperator::threshold_not;
    if (t == "yagerAnd")
      return ast::LogicalOperator::yager_and;
    if (t == "yagerNot")
      return ast::LogicalOperator::yager_not;
    return ast::LogicalOperator::yager_or;
  };
  struct LOGICAL_OP
      : terminal<kw<"and"> | kw<"boundedAnd"> | kw<"cosineNot"> |
                     kw<"meanAnd"> | kw<"meanOr"> | kw<"or"> |
                     kw<"productAnd"> | kw<"productOr"> | kw<"sugenoNot"> |
                     kw<"thresholdNot"> | kw<"yagerAnd"> | kw<"yagerNot"> |
                     kw<"yagerOr">,
                 cursiv::text_conv<operator_conv>> {};

  struct AttribClauseRule;
  struct OperandRule;
  struct ClauseChain;
  struct ClauseRule;

  struct AttribClauseRule
      : rule<create<ast::AttribClause>(
            assign<&ast::AttribClause::subject>(ref<IDENTIFIER>) +
            (flag<&ast::AttribClause::plural>(kw<"are">) |
             kw<"s">)+many(append<&ast::AttribClause::hedges>(ref<HEDGE>)) +
            (assign<&ast::AttribClause::value>(ref<FP_LITERAL>) |
             assign<&ast::AttribClause::value>(ref<IDENTIFIER>) |
             assign<&ast::AttribClause::value>(ref<RESTRICTION>)))> {};

  // Parens build no node: the rule adopts what its body produced.
  struct OperandRule
      : rule<(lit<"("> + ref<ClauseRule> + lit<")">) | ref<AttribClauseRule>> {
  };

  // One flat, left-associative level: every fuzzy operator ties operands at
  // the same precedence, so the ladder has no second level to order against.
  struct ClauseChain
      : infix<OperandRule, &ast::LogicalExpr::left, &ast::LogicalExpr::op,
              &ast::LogicalExpr::right, left(ref<LOGICAL_OP>)> {};

  struct ClauseRule : rule<ref<ClauseChain>> {};

  struct ConsequentRule
      : rule<create<ast::Consequent>(
            kw<"then"> +
            assign<&ast::Consequent::thenClause>(ref<AttribClauseRule>) +
            option(kw<"else"> + assign<&ast::Consequent::elseClause>(
                                    ref<AttribClauseRule>)))> {};

  struct ConditionalRule
      : rule<create<ast::ConditionalRule>(
            assign<&ast::ConditionalRule::name>(ref<IDENTIFIER>) + kw<"if"> +
            assign<&ast::ConditionalRule::premise>(ref<ClauseRule>) +
            assign<&ast::ConditionalRule::consequent>(ref<ConsequentRule>) +
            lit<";">)> {};

  using entry = ConditionalRule;
  using ignore = WS;
};

} // namespace infosapient
