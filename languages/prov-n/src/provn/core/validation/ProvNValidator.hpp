// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "provn/core/ast.hpp"

namespace provn {

class ProvNValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ProvNCoreServices;
void register_validation_checks(ProvNCoreServices& services);

}  // namespace provn
