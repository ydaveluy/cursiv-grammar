// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "tnsnames/ast.hpp"

namespace tnsnames {

class TnsnamesValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TnsnamesCoreServices;
void register_validation_checks(TnsnamesCoreServices& services);

}  // namespace tnsnames
