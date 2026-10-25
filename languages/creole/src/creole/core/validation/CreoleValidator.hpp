// The checks of this language: what the grammar accepts but the language
// does not. None yet -- creole has no cross-reference and no naming rule to
// enforce.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "creole/core/ast.hpp"

namespace creole {

class CreoleValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CreoleCoreServices;
void register_validation_checks(CreoleCoreServices& services);

}  // namespace creole
