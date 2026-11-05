#include "chip8/validation/Chip8Validator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "chip8/CoreServices.hpp"

namespace chip8 {

void register_validation_checks(Chip8CoreServices& services) {
  using V = Chip8Validator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace chip8
