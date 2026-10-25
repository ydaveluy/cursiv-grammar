#include "provn/core/validation/ProvNValidator.hpp"

#include <memory>

#include "provn/core/CoreServices.hpp"

namespace provn {

void register_validation_checks(ProvNCoreServices& services) {
  services.validator = std::make_unique<const ProvNValidator>(services);
}

}  // namespace provn
