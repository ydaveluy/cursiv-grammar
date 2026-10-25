#include "trac/core/validation/TracValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "trac/core/CoreServices.hpp"

namespace trac {

void register_validation_checks(TracCoreServices& services) {
  services.validator = std::make_unique<const TracValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace trac
