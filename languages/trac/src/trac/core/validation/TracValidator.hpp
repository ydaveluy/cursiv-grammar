// The checks of this language: what the grammar accepts but the language
// does not. This teaching subset does not resolve or execute macro names.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "trac/core/ast.hpp"

namespace trac {

class TracValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TracCoreServices;
void register_validation_checks(TracCoreServices& services);

}  // namespace trac
