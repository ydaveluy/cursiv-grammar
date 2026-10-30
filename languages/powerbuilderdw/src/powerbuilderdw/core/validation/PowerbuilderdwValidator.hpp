// Literal spelling is preserved; floating range failures are diagnosed.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "powerbuilderdw/core/ast.hpp"

namespace powerbuilderdw {

class PowerbuilderdwValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::NumberValue&, cursiv::ValidationAcceptor&) const;
};

struct PowerbuilderdwCoreServices;
void register_validation_checks(PowerbuilderdwCoreServices& services);

}  // namespace powerbuilderdw
