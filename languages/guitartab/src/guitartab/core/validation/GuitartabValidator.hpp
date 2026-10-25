// The checks of this language. None yet: a guitar tab has no naming
// convention and no cross-reference for a check to judge.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "guitartab/core/ast.hpp"

namespace guitartab {

class GuitartabValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct GuitartabCoreServices;
void register_validation_checks(GuitartabCoreServices& services);

}  // namespace guitartab
