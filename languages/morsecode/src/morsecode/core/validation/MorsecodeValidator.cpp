#include "morsecode/core/validation/MorsecodeValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "morsecode/core/CoreServices.hpp"

namespace morsecode {

void register_validation_checks(MorsecodeCoreServices& services) {
  using V = MorsecodeValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace morsecode
