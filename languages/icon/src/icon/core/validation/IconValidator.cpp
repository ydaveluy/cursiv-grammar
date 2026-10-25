#include "icon/core/validation/IconValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "icon/core/CoreServices.hpp"

namespace icon {

void register_validation_checks(IconCoreServices& services) {
  services.validator = std::make_unique<const IconValidator>(services);
}

}  // namespace icon
