// The checks of this language: what the grammar accepts but the language
// does not. None yet -- useragent has no names and no semantic constraints.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace useragent {

class UseragentValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct UseragentCoreServices;
void register_validation_checks(UseragentCoreServices& services);

}  // namespace useragent
