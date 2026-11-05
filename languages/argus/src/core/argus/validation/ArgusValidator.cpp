#include "argus/validation/ArgusValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "argus/CoreServices.hpp"

namespace argus {

void register_validation_checks(ArgusCoreServices& services) {
  services.validator = std::make_unique<const ArgusValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace argus
