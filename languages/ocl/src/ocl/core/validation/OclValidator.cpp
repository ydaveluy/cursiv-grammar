#include "ocl/core/validation/OclValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "ocl/core/CoreServices.hpp"

namespace ocl {

void register_validation_checks(OclCoreServices& services) {
  services.validator = std::make_unique<const OclValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace ocl
