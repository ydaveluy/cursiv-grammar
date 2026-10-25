#include "php/core/validation/PhpValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "php/core/CoreServices.hpp"

namespace php {

void register_validation_checks(PhpCoreServices& services) {
  using V = PhpValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace php
