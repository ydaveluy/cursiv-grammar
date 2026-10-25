// The checks of this language: none yet -- nothing here needs a check
// beyond what parsing already enforces.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace mumath {

class MumathValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct MumathCoreServices;
void register_validation_checks(MumathCoreServices& services);

}  // namespace mumath
