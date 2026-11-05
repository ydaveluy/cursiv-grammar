#include "mumath/validation/MumathValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "mumath/CoreServices.hpp"

namespace mumath {

void register_validation_checks(MumathCoreServices& services) {
  using V = MumathValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace mumath
