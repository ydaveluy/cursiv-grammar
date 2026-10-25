#include "calculator/core/validation/CalculatorValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "calculator/core/CoreServices.hpp"

namespace calculator {

void register_validation_checks(CalculatorCoreServices& services) {
  services.validator = std::make_unique<const CalculatorValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace calculator
