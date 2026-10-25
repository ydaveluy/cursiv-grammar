// The checks of this language: none yet, beyond what the grammar enforces.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace tinymud {

class TinymudValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TinymudCoreServices;
void register_validation_checks(TinymudCoreServices& services);

}  // namespace tinymud
