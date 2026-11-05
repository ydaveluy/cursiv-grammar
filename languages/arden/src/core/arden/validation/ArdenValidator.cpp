#include "arden/validation/ArdenValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "arden/CoreServices.hpp"

namespace arden {

void register_validation_checks(ArdenCoreServices& services) {
  services.validator = std::make_unique<const ArdenValidator>(services);
}

}  // namespace arden
