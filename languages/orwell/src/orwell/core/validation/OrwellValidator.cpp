#include "orwell/core/validation/OrwellValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "orwell/core/CoreServices.hpp"

namespace orwell {

void register_validation_checks(OrwellCoreServices& services) {
  services.validator =
      std::make_unique<const OrwellValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace orwell
