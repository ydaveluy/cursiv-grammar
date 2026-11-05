#include "infosapient/validation/InfosapientValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "infosapient/CoreServices.hpp"

namespace infosapient {

void register_validation_checks(InfosapientCoreServices& services) {
  using V = InfosapientValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace infosapient
