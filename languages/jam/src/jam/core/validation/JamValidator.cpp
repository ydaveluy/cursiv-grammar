#include "jam/core/validation/JamValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "jam/core/CoreServices.hpp"

namespace jam {

void register_validation_checks(JamCoreServices& services) {
  services.validator = std::make_unique<const JamValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace jam
