#include "rpn/core/validation/RpnValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "rpn/core/CoreServices.hpp"

namespace rpn {

void register_validation_checks(RpnCoreServices& services) {
  services.validator = std::make_unique<const RpnValidator>(services);
}

}  // namespace rpn
