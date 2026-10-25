// Parameter order is checked after parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "powerquery/core/ast.hpp"

namespace powerquery {

class PowerqueryValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkTypeParamOrder(const ast::FunctionTypeNode&,
                           cursiv::ValidationAcceptor&) const;
  void checkParamOrder(const ast::FunctionExpr&,
                       cursiv::ValidationAcceptor&) const;
};

struct PowerqueryCoreServices;
void register_validation_checks(PowerqueryCoreServices& services);

}  // namespace powerquery
