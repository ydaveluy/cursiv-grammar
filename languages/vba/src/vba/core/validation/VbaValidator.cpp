#include "vba/core/validation/VbaValidator.hpp"

#include <memory>

#include "vba/core/CoreServices.hpp"

namespace vba {

void register_validation_checks(VbaCoreServices& services) {
  services.validator = std::make_unique<const VbaValidator>(services);
}

}  // namespace vba
