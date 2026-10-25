// The checks of this language: none -- the grammar has no naming or
// resolution rule the parser does not already enforce.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace d2 {

class D2Validator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct D2CoreServices;
void register_validation_checks(D2CoreServices& services);

}  // namespace d2
