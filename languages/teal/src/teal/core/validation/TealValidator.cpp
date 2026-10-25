#include "teal/core/validation/TealValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "teal/core/CoreServices.hpp"

namespace teal {

void register_validation_checks(TealCoreServices& services) {
  services.validator = std::make_unique<const TealValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace teal
