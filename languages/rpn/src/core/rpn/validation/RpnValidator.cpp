#include "rpn/validation/RpnValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "rpn/CoreServices.hpp"

namespace rpn {

void register_validation_checks(RpnCoreServices& services) {
  services.validator = std::make_unique<const RpnValidator>(services);
}

}  // namespace rpn
