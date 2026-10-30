// Modifier targets are checked after parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "promql/core/ast.hpp"

namespace promql {

class PromqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDuration(const ast::DurationLit&, cursiv::ValidationAcceptor&) const;
  void checkFunction(const ast::FunctionCall&, cursiv::ValidationAcceptor&) const;
  void checkAggregation(const ast::AggregationExpr&, cursiv::ValidationAcceptor&) const;
  void checkBinary(const ast::BinaryExpr&, cursiv::ValidationAcceptor&) const;
  void checkUnary(const ast::UnaryExpr&, cursiv::ValidationAcceptor&) const;
  void checkSubqueryType(const ast::Subquery&, cursiv::ValidationAcceptor&) const;
  void checkSelector(const ast::Selector&, cursiv::ValidationAcceptor&) const;
  void checkRangeModifier(const ast::RangeModifier&, cursiv::ValidationAcceptor&) const;
  void checkSelectorDurations(const ast::Selector&,
                              cursiv::ValidationAcceptor&) const;
  void checkSubqueryDurations(const ast::Subquery&,
                              cursiv::ValidationAcceptor&) const;
  void checkTimestampTarget(const ast::Timestamped&,
                            cursiv::ValidationAcceptor&) const;
};

struct PromqlCoreServices;
void register_validation_checks(PromqlCoreServices& services);

}  // namespace promql
