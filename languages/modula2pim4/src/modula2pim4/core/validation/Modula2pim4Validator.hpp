// The checks of this language. None yet: this is a syntax-only port.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace modula2pim4 {

class Modula2pim4Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct Modula2pim4CoreServices;
void register_validation_checks(Modula2pim4CoreServices& services);

}  // namespace modula2pim4
