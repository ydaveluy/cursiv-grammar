// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "corundum/ast.hpp"

namespace corundum {

class CorundumValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CorundumCoreServices;
void register_validation_checks(CorundumCoreServices& services);

}  // namespace corundum
