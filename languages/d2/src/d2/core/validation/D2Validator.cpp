#include "d2/core/validation/D2Validator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "d2/core/CoreServices.hpp"

namespace d2 {

void register_validation_checks(D2CoreServices& services) {
  services.validator = std::make_unique<const D2Validator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace d2
