// The checks of this language: what the grammar accepts but the language does
// not. None yet -- the grammar's own ambiguities (twin `prefix`/`infix`
// sections, the unreachable reference `DIGIT`/`CHAR` tokens) are handled in
// the grammar itself, not here.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "orwell/core/ast.hpp"

namespace orwell {

class OrwellValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct OrwellCoreServices;
void register_validation_checks(OrwellCoreServices& services);

}  // namespace orwell
