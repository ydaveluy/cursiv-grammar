#include "focal/validation/FocalValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "focal/CoreServices.hpp"

namespace focal {

void register_validation_checks(FocalCoreServices& services) {
  services.validator = std::make_unique<const FocalValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace focal
