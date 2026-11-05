#include "orwell/validation/OrwellValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "orwell/CoreServices.hpp"

namespace orwell {

void register_validation_checks(OrwellCoreServices& services) {
  services.validator =
      std::make_unique<const OrwellValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace orwell
