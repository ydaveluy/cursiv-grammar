#include "bcl/core/validation/BclValidator.hpp"

#include <memory>

#include "bcl/core/CoreServices.hpp"

namespace bcl {

void register_validation_checks(BclCoreServices& services) {
  services.validator = std::make_unique<const BclValidator>(services);
}

}  // namespace bcl
