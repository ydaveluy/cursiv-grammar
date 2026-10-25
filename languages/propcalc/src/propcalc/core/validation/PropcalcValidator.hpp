#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "propcalc/core/ast.hpp"

namespace propcalc {

class PropcalcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkVariable(const ast::VariableRef&, cursiv::ValidationAcceptor&) const;
};

struct PropcalcCoreServices;
void register_validation_checks(PropcalcCoreServices& services);

}  // namespace propcalc
