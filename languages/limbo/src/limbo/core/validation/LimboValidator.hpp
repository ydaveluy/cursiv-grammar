// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "limbo/core/ast.hpp"

namespace limbo {

class LimboValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct LimboCoreServices;
void register_validation_checks(LimboCoreServices& services);

}  // namespace limbo
