// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "corundum/core/ast.hpp"

namespace corundum {

class CorundumValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CorundumCoreServices;
void register_validation_checks(CorundumCoreServices& services);

}  // namespace corundum
