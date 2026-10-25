// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "tnsnames/core/ast.hpp"

namespace tnsnames {

class TnsnamesValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TnsnamesCoreServices;
void register_validation_checks(TnsnamesCoreServices& services);

}  // namespace tnsnames
