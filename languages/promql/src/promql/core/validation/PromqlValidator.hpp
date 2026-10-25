// Modifier targets are checked after parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "promql/core/ast.hpp"

namespace promql {

class PromqlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
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
