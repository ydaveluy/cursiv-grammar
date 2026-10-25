// The checks of this language: what the grammar accepts but the language does
// not. None yet.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "kirikiritjs/core/ast.hpp"

namespace kirikiritjs {

class KirikiriTjsValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct KirikiriTjsCoreServices;
void register_validation_checks(KirikiriTjsCoreServices& services);

}  // namespace kirikiritjs
