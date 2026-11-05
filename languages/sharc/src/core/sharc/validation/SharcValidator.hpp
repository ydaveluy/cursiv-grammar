// Directive-local checks; native encoding, symbol and section constraints
// require a separate semantic pass.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "sharc/ast.hpp"

namespace sharc {

class SharcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDirective(const ast::Directive&, cursiv::ValidationAcceptor&) const;
};

struct SharcCoreServices;
void register_validation_checks(SharcCoreServices& services);

}  // namespace sharc
