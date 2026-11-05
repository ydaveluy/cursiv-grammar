#include "trac/validation/TracValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "trac/CoreServices.hpp"

namespace trac {

void register_validation_checks(TracCoreServices& services) {
  services.validator = std::make_unique<const TracValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace trac
