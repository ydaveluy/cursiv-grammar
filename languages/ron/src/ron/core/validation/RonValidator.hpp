// The checks of this language: none so far -- ron has no name resolution
// and no cross-node fact to police, so the registry stays empty.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "ron/core/ast.hpp"

namespace ron {

class RonValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct RonCoreServices;
void register_validation_checks(RonCoreServices& services);

}  // namespace ron
