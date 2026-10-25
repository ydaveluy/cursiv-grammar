// The checks of this language: none yet. Nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "vba/core/ast.hpp"

namespace vba {

class VbaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct VbaCoreServices;
void register_validation_checks(VbaCoreServices& services);

}  // namespace vba
