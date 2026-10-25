#include "arden/core/validation/ArdenValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "arden/core/CoreServices.hpp"

namespace arden {

void register_validation_checks(ArdenCoreServices& services) {
  services.validator = std::make_unique<const ArdenValidator>(services);
}

}  // namespace arden
