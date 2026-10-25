#include "basic/core/validation/BasicValidator.hpp"

#include <memory>

#include "basic/core/CoreServices.hpp"

namespace basic {

void register_validation_checks(BasicCoreServices& services) {
  services.validator = std::make_unique<const BasicValidator>(services);
}

}  // namespace basic
