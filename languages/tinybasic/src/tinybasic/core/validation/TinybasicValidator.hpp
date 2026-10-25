// The checks of this language: what the grammar accepts but the language
// does not. None yet -- tinybasic has no names and no semantic constraints.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace tinybasic {

class TinybasicValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TinybasicCoreServices;
void register_validation_checks(TinybasicCoreServices& services);

}  // namespace tinybasic
