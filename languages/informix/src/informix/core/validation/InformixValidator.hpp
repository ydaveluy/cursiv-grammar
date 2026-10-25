// Validation checks for informix. None yet.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace informix {

class InformixValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct InformixCoreServices;
void register_validation_checks(InformixCoreServices& services);

}  // namespace informix
