// The checks of this language: none yet, beyond what the grammar itself
// enforces by shape.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "squirrel/core/ast.hpp"

namespace squirrel {

class SquirrelValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SquirrelCoreServices;
void register_validation_checks(SquirrelCoreServices& services);

}  // namespace squirrel
