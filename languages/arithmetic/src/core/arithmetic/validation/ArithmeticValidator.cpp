#include "arithmetic/validation/ArithmeticValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "arithmetic/CoreServices.hpp"

namespace arithmetic {

void register_validation_checks(ArithmeticCoreServices& services) {
  services.validator = std::make_unique<const ArithmeticValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace arithmetic
