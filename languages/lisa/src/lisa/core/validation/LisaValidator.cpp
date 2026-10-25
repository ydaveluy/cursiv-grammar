#include "lisa/core/validation/LisaValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lisa/core/CoreServices.hpp"

namespace lisa {

void register_validation_checks(LisaCoreServices& services) {
  services.validator = std::make_unique<const LisaValidator>(services);
}

}  // namespace lisa
