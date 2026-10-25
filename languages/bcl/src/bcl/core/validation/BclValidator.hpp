// The checks of this language: what the grammar accepts but the language
// does not. None yet -- bcl has no names and no semantic constraints.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace bcl {

class BclValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct BclCoreServices;
void register_validation_checks(BclCoreServices& services);

}  // namespace bcl
