// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "powerbuilderdw/core/ast.hpp"

namespace powerbuilderdw {

class PowerbuilderdwValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PowerbuilderdwCoreServices;
void register_validation_checks(PowerbuilderdwCoreServices& services);

}  // namespace powerbuilderdw
