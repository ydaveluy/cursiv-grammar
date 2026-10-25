// The checks of this language: what the grammar accepts but the language
// does not. None yet: nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "teal/core/ast.hpp"

namespace teal {

class TealValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TealCoreServices;
void register_validation_checks(TealCoreServices& services);

}  // namespace teal
