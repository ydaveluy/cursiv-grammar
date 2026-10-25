// The checks of this language: what the grammar accepts but the language
// does not. None yet -- every call the grammar builds is a valid document.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace ttm {

class TtmValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TtmCoreServices;
void register_validation_checks(TtmCoreServices& services);

}  // namespace ttm
