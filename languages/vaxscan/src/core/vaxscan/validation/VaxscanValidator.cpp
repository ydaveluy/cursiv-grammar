#include "vaxscan/validation/VaxscanValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "vaxscan/CoreServices.hpp"

namespace vaxscan {

// None yet: nothing here needs a check beyond parsing.
void register_validation_checks(VaxscanCoreServices& services) {
  using V = VaxscanValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace vaxscan
