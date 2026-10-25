// The checks of this language. The grammar's own acceptance already is the
// language: no symbol table, no cross-reference, so no check is registered.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace redcode {

class RedcodeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct RedcodeCoreServices;
void register_validation_checks(RedcodeCoreServices& services);

}  // namespace redcode
