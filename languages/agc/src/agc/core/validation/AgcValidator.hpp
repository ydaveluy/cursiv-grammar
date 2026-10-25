// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "agc/core/ast.hpp"

namespace agc {

class AgcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct AgcCoreServices;
void register_validation_checks(AgcCoreServices& services);

}  // namespace agc
