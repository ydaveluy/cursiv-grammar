#pragma once

#include <cmath>
#include <cstdio>
#include <string_view>
#include <utility>

#include "cursiv/core/parser/Parser.hpp"
#include "promql/core/ast.hpp"

namespace promql_audit {

inline bool run(const cursiv::Parser& parser) {
  using namespace promql::ast;
  int failures = 0;
  const auto check = [&](bool ok) { if (!ok) ++failures; };
  const auto parse = [&](std::string_view text) {
    auto result = parser.parse(text);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty());
    return result;
  };
  for (const auto& [text, expected] : {
           std::pair<std::string_view, double>{".5", .5}, {"1.", 1},
           {"0xFf", 255}, {"077", 63}, {"1_000", 1000}}) {
    const auto result = parse(text);
    const auto* query = result.value_as<Query>();
    const auto* number = query ? cursiv::ast_cast<NumberLit>(query->expression) : nullptr;
    check(number && number->value == expected);
  }
  for (const std::string_view text : {"Inf", "NaN"}) {
    const auto result = parse(text);
    const auto* query = result.value_as<Query>();
    const auto* number = query ? cursiv::ast_cast<NumberLit>(query->expression) : nullptr;
    check(number && (text == "Inf" ? std::isinf(number->value) : std::isnan(number->value)));
  }
  for (const auto& [text, expected] : {
           std::pair<std::string_view, std::string_view>{R"("\a\b\f\n\r\t\v\x41\101\u00e9\U0001F600")", "\a\b\f\n\r\t\vAAé😀"},
           {"`a\\n\nb\r`", "a\\n\nb\r"}}) {
    const auto result = parse(text);
    const auto* query = result.value_as<Query>();
    const auto* string = query ? cursiv::ast_cast<StringLit>(query->expression) : nullptr;
    check(string && string->value == expected);
  }
  for (const std::string_view text : {"foo[STEP()]", "foo[RANGE()]",
                                      "foo[MAX_OF(1m,2m)]", "foo offset MIN_OF(1m,2m)"})
    parse(text);
  {
    const auto result = parse("info(up)");
    const auto* query = result.value_as<Query>();
    const auto* call = query ? cursiv::ast_cast<FunctionCall>(query->expression) : nullptr;
    check(call && call->name == "info" && call->args.size() == 1);
  }
  for (const auto& [text, expected] : {
           std::pair<std::string_view, std::string_view>{"foo offset -5m", "-5m"},
           {"foo offset +5m", "+5m"}}) {
    const auto result = parse(text);
    const auto* query = result.value_as<Query>();
    const auto* selector = query ? cursiv::ast_cast<Selector>(query->expression) : nullptr;
    const auto* unary = selector ? cursiv::ast_cast<UnaryExpr>(selector->offset) : nullptr;
    const auto* duration = unary ? cursiv::ast_cast<DurationLit>(unary->operand) : nullptr;
    check(selector && selector->metric == "foo" && duration && duration->text == "5m" &&
          unary->op == (expected.front() == '-' ? Operator::subtract : Operator::add));
  }
  {
    const auto result = parse("sum by (job,)(up)");
    const auto* query = result.value_as<Query>();
    const auto* agg = query ? cursiv::ast_cast<AggregationExpr>(query->expression) : nullptr;
    check(agg && agg->hasGrouping && agg->groupBy.size() == 1 && agg->groupBy[0] == "job");
  }
  {
    const auto result = parse("foo + on(job,) group_left(x,) bar");
    const auto* query = result.value_as<Query>();
    const auto* binary = query ? cursiv::ast_cast<BinaryExpr>(query->expression) : nullptr;
    check(binary && binary->matching && binary->matching->labels.size() == 1 && binary->matching->sideLabels.size() == 1);
  }
  {
    const auto result = parse("{\"métric\", \"étiquette\"=\"été\"}");
    const auto* query = result.value_as<Query>();
    const auto* selector = query ? cursiv::ast_cast<Selector>(query->expression) : nullptr;
    check(selector && selector->metric.empty() && selector->matchers.size() == 2 &&
          selector->matchers[0]->name == "__name__" && selector->matchers[0]->value == "métric" &&
          selector->matchers[1]->name == "étiquette" && selector->matchers[1]->value == "été");
  }
  {
    const auto result = parse("atan2_metric atan2 another");
    const auto* query = result.value_as<Query>();
    const auto* binary = query ? cursiv::ast_cast<BinaryExpr>(query->expression) : nullptr;
    check(binary && binary->op == Operator::atan2);
  }
  {
    const auto result = parse("#\rfoo");
    const auto* query = result.value_as<Query>();
    const auto* selector = query ? cursiv::ast_cast<Selector>(query->expression) : nullptr;
    check(selector && selector->metric == "foo");
  }
  {
    const auto result = parse("foo[11s+10s-5*2^2]");
    const auto* query = result.value_as<Query>();
    const auto* selector = query ? cursiv::ast_cast<Selector>(query->expression) : nullptr;
    const auto* sub = selector ? cursiv::ast_cast<BinaryExpr>(selector->range) : nullptr;
    const auto* sum = sub ? cursiv::ast_cast<BinaryExpr>(sub->left) : nullptr;
    const auto* product = sub ? cursiv::ast_cast<BinaryExpr>(sub->right) : nullptr;
    check(sub && sub->parent == selector && sub->op == Operator::subtract &&
          sum && sum->op == Operator::add && product && product->op == Operator::multiply);
    const auto* power = product ? cursiv::ast_cast<BinaryExpr>(product->right) : nullptr;
    const auto* duration = sum ? cursiv::ast_cast<DurationLit>(sum->left) : nullptr;
    check(power && power->op == Operator::pow && duration && duration->text == "11s");
  }
  {
    const auto result = parse("foo[range():step()] offset -(1^2)");
    const auto* query = result.value_as<Query>();
    const auto* subquery = query ? cursiv::ast_cast<Subquery>(query->expression) : nullptr;
    const auto* range = subquery ? cursiv::ast_cast<FunctionCall>(subquery->range) : nullptr;
    const auto* step = subquery ? cursiv::ast_cast<FunctionCall>(subquery->step) : nullptr;
    const auto* offset = subquery ? cursiv::ast_cast<UnaryExpr>(subquery->offset) : nullptr;
    check(range && range->name == "range" && range->parent == subquery &&
          step && step->name == "step" && offset && offset->op == Operator::subtract);
    const auto* power = offset ? cursiv::ast_cast<BinaryExpr>(offset->operand) : nullptr;
    check(power && power->op == Operator::pow);
  }
  for (const std::string_view source : {"foo </ bar", "foo >/ bar"}) {
    const auto result = parse(source);
    const auto* query = result.value_as<Query>();
    const auto* binary = query ? cursiv::ast_cast<BinaryExpr>(query->expression) : nullptr;
    check(binary && binary->op == (source[4] == '<' ? Operator::trim_upper : Operator::trim_lower));
  }
  {
    const auto result = parse("foo + on(job) fill_left(1) fill_right(-2) bar");
    const auto* query = result.value_as<Query>();
    const auto* binary = query ? cursiv::ast_cast<BinaryExpr>(query->expression) : nullptr;
    check(binary && binary->matching && binary->fills.size() == 2);
    if (binary && binary->fills.size() == 2) {
      const auto* left = binary->fills[0];
      const auto* right = binary->fills[1];
      check(left->name == "fill_left" && right->name == "fill_right" &&
            left->parent == binary && right->parent == binary &&
            cursiv::ast_cast<NumberLit>(left->value) && cursiv::ast_cast<UnaryExpr>(right->value));
    }
  }
  for (const std::string_view source : {"rate(foo[5m] anchored)", "rate(foo[5m] smoothed)"}) {
    const auto result = parse(source);
    const auto* query = result.value_as<Query>();
    const auto* call = query ? cursiv::ast_cast<FunctionCall>(query->expression) : nullptr;
    const auto* modifier = call && call->args.size() == 1 ? cursiv::ast_cast<RangeModifier>(call->args[0]) : nullptr;
    check(modifier && (modifier->name == "anchored" || modifier->name == "smoothed") &&
          cursiv::ast_cast<Selector>(modifier->operand));
  }
  if (failures) std::fprintf(stderr, "promql: %d AST audit failures\n", failures);
  return failures == 0;
}

}  // namespace promql_audit
