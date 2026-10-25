#include "focal/core/validation/FocalValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "focal/core/CoreServices.hpp"

namespace focal {

void register_validation_checks(FocalCoreServices& services) {
  services.validator = std::make_unique<const FocalValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace focal
