// The checks of this language: what the grammar accepts but the language
// does not. None yet: nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace matlab {

class MatlabValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct MatlabCoreServices;
void register_validation_checks(MatlabCoreServices& services);

}  // namespace matlab
