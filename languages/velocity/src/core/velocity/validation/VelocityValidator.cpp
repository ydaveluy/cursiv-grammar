#include "velocity/validation/VelocityValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "velocity/CoreServices.hpp"

namespace velocity {

void register_validation_checks(VelocityCoreServices& services) {
  using V = VelocityValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace velocity
