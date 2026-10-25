#include "velocity/core/validation/VelocityValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "velocity/core/CoreServices.hpp"

namespace velocity {

void register_validation_checks(VelocityCoreServices& services) {
  using V = VelocityValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace velocity
