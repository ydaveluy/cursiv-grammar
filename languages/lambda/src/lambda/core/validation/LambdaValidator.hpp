// The checks of this language: what the grammar accepts but the language
// does not. None yet -- a free variable is an open term, valid on its own.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace lambda {

class LambdaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct LambdaCoreServices;
void register_validation_checks(LambdaCoreServices& services);

}  // namespace lambda
