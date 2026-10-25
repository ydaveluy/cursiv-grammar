// The checks of this language. None yet: nothing here needs a check beyond
// parsing, and the grammar's own ambiguities are resolved by the parser.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "argus/core/ast.hpp"

namespace argus {

class ArgusValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ArgusCoreServices;
void register_validation_checks(ArgusCoreServices& services);

}  // namespace argus
