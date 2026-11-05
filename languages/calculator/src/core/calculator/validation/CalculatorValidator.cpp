#include "calculator/validation/CalculatorValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "calculator/CoreServices.hpp"

namespace calculator {

void register_validation_checks(CalculatorCoreServices& services) {
  services.validator = std::make_unique<const CalculatorValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace calculator
