#include "chip8/core/validation/Chip8Validator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "chip8/core/CoreServices.hpp"

namespace chip8 {

void register_validation_checks(Chip8CoreServices& services) {
  using V = Chip8Validator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace chip8
