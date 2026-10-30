// No semantic checks are registered: native name/type/loop-scope/error-code
// and database-dependent command rules need further validation.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "moo/core/ast.hpp"

namespace moo {

class MooValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct MooCoreServices;
void register_validation_checks(MooCoreServices& services);

}  // namespace moo
