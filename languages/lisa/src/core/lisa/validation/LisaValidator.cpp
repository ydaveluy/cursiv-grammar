#include "lisa/validation/LisaValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "lisa/CoreServices.hpp"

namespace lisa {

void register_validation_checks(LisaCoreServices& services) {
  services.validator = std::make_unique<const LisaValidator>(services);
}

}  // namespace lisa
