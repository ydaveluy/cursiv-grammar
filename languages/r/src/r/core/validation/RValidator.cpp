#include "r/core/validation/RValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "r/core/CoreServices.hpp"

namespace r {

void register_validation_checks(RCoreServices& services) {
  services.validator = std::make_unique<const RValidator>(services);
}

}  // namespace r
