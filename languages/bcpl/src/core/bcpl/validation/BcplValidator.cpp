#include "bcpl/validation/BcplValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "bcpl/CoreServices.hpp"

namespace bcpl {

void register_validation_checks(BcplCoreServices& services) {
  services.validator = std::make_unique<const BcplValidator>(services);
}

}  // namespace bcpl
