#include "bicep/validation/BicepValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "bicep/CoreServices.hpp"

namespace bicep {

void register_validation_checks(BicepCoreServices& services) {
  using V = BicepValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace bicep
