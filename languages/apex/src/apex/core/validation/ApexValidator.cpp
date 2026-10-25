#include "apex/core/validation/ApexValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "apex/core/CoreServices.hpp"

namespace apex {

void register_validation_checks(ApexCoreServices& services) {
  services.validator = std::make_unique<const ApexValidator>(services);
}

}  // namespace apex
