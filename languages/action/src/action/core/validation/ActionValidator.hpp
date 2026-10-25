// The checks of this language: none yet.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace action {

class ActionValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ActionCoreServices;
void register_validation_checks(ActionCoreServices& services);

}  // namespace action
