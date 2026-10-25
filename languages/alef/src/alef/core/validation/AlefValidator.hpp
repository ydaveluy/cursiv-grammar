// The checks of this language: none yet, everything here parses.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "alef/core/ast.hpp"

namespace alef {

class AlefValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct AlefCoreServices;
void register_validation_checks(AlefCoreServices& services);

}  // namespace alef
