#include "bcpl/core/validation/BcplValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "bcpl/core/CoreServices.hpp"

namespace bcpl {

void register_validation_checks(BcplCoreServices& services) {
  services.validator = std::make_unique<const BcplValidator>(services);
}

}  // namespace bcpl
