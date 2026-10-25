// The checks of this language. None yet: the port covers syntax only.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "velocity/core/ast.hpp"

namespace velocity {

class VelocityValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct VelocityCoreServices;
void register_validation_checks(VelocityCoreServices& services);

}  // namespace velocity
