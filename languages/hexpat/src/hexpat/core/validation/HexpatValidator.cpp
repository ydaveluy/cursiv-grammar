#include "hexpat/core/validation/HexpatValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "hexpat/core/CoreServices.hpp"

namespace hexpat {

void register_validation_checks(HexpatCoreServices& services) {
  using V = HexpatValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace hexpat
