#include "pmmn/core/validation/PmmnValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pmmn/core/CoreServices.hpp"

namespace pmmn {

void register_validation_checks(PmmnCoreServices& services) {
  services.validator = std::make_unique<const PmmnValidator>(services);
}

}  // namespace pmmn
