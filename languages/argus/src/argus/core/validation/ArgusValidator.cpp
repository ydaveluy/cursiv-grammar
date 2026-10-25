#include "argus/core/validation/ArgusValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "argus/core/CoreServices.hpp"

namespace argus {

void register_validation_checks(ArgusCoreServices& services) {
  services.validator = std::make_unique<const ArgusValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace argus
