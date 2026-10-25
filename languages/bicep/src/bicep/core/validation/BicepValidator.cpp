#include "bicep/core/validation/BicepValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "bicep/core/CoreServices.hpp"

namespace bicep {

void register_validation_checks(BicepCoreServices& services) {
  using V = BicepValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace bicep
