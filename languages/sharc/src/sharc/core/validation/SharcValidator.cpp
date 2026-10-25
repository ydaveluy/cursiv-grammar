#include "sharc/core/validation/SharcValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sharc/core/CoreServices.hpp"

namespace sharc {

void register_validation_checks(SharcCoreServices& services) {
  services.validator = std::make_unique<const SharcValidator>(services);
}

}  // namespace sharc
