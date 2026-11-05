#include "apex/validation/ApexValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "apex/CoreServices.hpp"

namespace apex {

void register_validation_checks(ApexCoreServices& services) {
  services.validator = std::make_unique<const ApexValidator>(services);
}

}  // namespace apex
