#include "useragent/core/validation/UseragentValidator.hpp"

#include <memory>

#include "useragent/core/CoreServices.hpp"

namespace useragent {

void register_validation_checks(UseragentCoreServices& services) {
  services.validator = std::make_unique<const UseragentValidator>(services);
}

}  // namespace useragent
