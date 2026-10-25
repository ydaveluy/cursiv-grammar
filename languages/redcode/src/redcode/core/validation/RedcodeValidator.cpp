#include "redcode/core/validation/RedcodeValidator.hpp"

#include <memory>

#include "redcode/core/CoreServices.hpp"

namespace redcode {

void register_validation_checks(RedcodeCoreServices& services) {
  services.validator = std::make_unique<const RedcodeValidator>(services);
}

}  // namespace redcode
