#include "logo/core/validation/LogoValidator.hpp"

#include <memory>

#include "logo/core/CoreServices.hpp"

namespace logo {

void register_validation_checks(LogoCoreServices& services) {
  services.validator = std::make_unique<const LogoValidator>(services);
}

}  // namespace logo
