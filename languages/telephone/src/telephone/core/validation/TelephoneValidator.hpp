// This teaching grammar does not describe a national numbering plan.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace telephone {

class TelephoneValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct TelephoneCoreServices;
void register_validation_checks(TelephoneCoreServices& services);

}  // namespace telephone
