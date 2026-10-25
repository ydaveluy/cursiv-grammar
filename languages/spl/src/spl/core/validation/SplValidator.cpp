#include "spl/core/validation/SplValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "spl/core/CoreServices.hpp"

namespace spl {

void register_validation_checks(SplCoreServices& services) {
  using V = SplValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace spl
