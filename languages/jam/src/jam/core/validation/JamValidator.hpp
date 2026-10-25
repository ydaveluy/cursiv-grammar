// The checks of this language: none -- a chord chart has no reference to
// resolve and no fact the grammar leaves for a validator to state.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace jam {

class JamValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct JamCoreServices;
void register_validation_checks(JamCoreServices& services);

}  // namespace jam
