#include "ron/validation/RonValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "ron/CoreServices.hpp"

namespace ron {

void register_validation_checks(RonCoreServices& services) {
  services.validator = std::make_unique<const RonValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace ron
