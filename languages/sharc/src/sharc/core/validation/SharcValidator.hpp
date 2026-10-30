// No semantic checks are registered yet. Native encoding, symbol and
// section constraints require a separate semantic pass.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "sharc/core/ast.hpp"

namespace sharc {

class SharcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SharcCoreServices;
void register_validation_checks(SharcCoreServices& services);

}  // namespace sharc
