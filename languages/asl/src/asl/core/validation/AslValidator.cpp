#include "asl/core/validation/AslValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "asl/core/CoreServices.hpp"

namespace asl {

// Nothing here yet: nothing beyond parsing needs a check.
void register_validation_checks(AslCoreServices& services) {
  using V = AslValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace asl
