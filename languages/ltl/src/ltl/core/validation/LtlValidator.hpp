// The checks of this language: none -- a proposition's shape is exactly what
// the grammar accepts, with no naming or scoping rule left to check.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace ltl {

class LtlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct LtlCoreServices;
void register_validation_checks(LtlCoreServices& services);

}  // namespace ltl
