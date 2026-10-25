#include "p/core/validation/PValidator.hpp"

#include <memory>

#include "p/core/CoreServices.hpp"

namespace p {

void register_validation_checks(PCoreServices& services) {
  services.validator = std::make_unique<const PValidator>(services);
}

}  // namespace p
