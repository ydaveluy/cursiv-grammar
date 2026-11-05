#include "alef/validation/AlefValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "alef/CoreServices.hpp"

namespace alef {

void register_validation_checks(AlefCoreServices& services) {
  using V = AlefValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace alef
