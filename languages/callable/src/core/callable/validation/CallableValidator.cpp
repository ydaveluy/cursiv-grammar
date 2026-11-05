#include "callable/validation/CallableValidator.hpp"

#include <memory>

#include "callable/CoreServices.hpp"

namespace callable {

void register_validation_checks(CallableCoreServices& services) {
  services.validator = std::make_unique<const CallableValidator>(services);
}

}  // namespace callable
