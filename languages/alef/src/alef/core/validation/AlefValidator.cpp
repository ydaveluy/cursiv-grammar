#include "alef/core/validation/AlefValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "alef/core/CoreServices.hpp"

namespace alef {

void register_validation_checks(AlefCoreServices& services) {
  using V = AlefValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace alef
