#include "corundum/validation/CorundumValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "corundum/CoreServices.hpp"

namespace corundum {

void register_validation_checks(CorundumCoreServices& services) {
  services.validator = std::make_unique<const CorundumValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace corundum
