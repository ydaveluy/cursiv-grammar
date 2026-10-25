#include "z/core/validation/ZValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "z/core/CoreServices.hpp"

namespace z {

void register_validation_checks(ZCoreServices& services) {
  services.validator = std::make_unique<const ZValidator>(services);
}

}  // namespace z
