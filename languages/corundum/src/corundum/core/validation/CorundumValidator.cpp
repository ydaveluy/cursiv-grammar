#include "corundum/core/validation/CorundumValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "corundum/core/CoreServices.hpp"

namespace corundum {

void register_validation_checks(CorundumCoreServices& services) {
  services.validator = std::make_unique<const CorundumValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace corundum
