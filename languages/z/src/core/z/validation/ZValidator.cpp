#include "z/validation/ZValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "z/CoreServices.hpp"

namespace z {

void register_validation_checks(ZCoreServices& services) {
  services.validator = std::make_unique<const ZValidator>(services);
}

}  // namespace z
