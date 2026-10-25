#include "v/core/validation/VValidator.hpp"

#include "v/core/CoreServices.hpp"

namespace v {

void register_validation_checks(VCoreServices& services) {
  services.validator = std::make_unique<const VValidator>(services);
}

}  // namespace v
