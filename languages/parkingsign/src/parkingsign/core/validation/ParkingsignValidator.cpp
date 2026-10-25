#include "parkingsign/core/validation/ParkingsignValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "parkingsign/core/CoreServices.hpp"

namespace parkingsign {

void register_validation_checks(ParkingsignCoreServices& services) {
  using V = ParkingsignValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace parkingsign
