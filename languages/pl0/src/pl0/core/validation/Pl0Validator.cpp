#include "pl0/core/validation/Pl0Validator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pl0/core/CoreServices.hpp"

namespace pl0 {

void register_validation_checks(Pl0CoreServices& services) {
  services.validator = std::make_unique<const Pl0Validator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace pl0
