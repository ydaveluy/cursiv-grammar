// The pedagogical guitartab.g4 notation has no added semantic checks here.
// Physical guitar tuning, rhythmic alignment and fret numbering are outside
// its mark-only language.
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
