#include "informix/core/validation/InformixValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "informix/core/CoreServices.hpp"

namespace informix {

// Registers nothing yet: the checks list is a hook for later.
void register_validation_checks(InformixCoreServices& services) {
  using V = InformixValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace informix
