#include "ron/core/validation/RonValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "ron/core/CoreServices.hpp"

namespace ron {

void register_validation_checks(RonCoreServices& services) {
  services.validator = std::make_unique<const RonValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace ron
