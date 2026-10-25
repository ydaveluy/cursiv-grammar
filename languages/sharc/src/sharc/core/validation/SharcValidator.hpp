// The checks of this language: none yet -- nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "sharc/core/ast.hpp"

namespace sharc {

class SharcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SharcCoreServices;
void register_validation_checks(SharcCoreServices& services);

}  // namespace sharc
