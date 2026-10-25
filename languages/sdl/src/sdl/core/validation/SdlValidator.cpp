#include "sdl/core/validation/SdlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sdl/core/CoreServices.hpp"

namespace sdl {

void register_validation_checks(SdlCoreServices& services) {
  using V = SdlValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace sdl
