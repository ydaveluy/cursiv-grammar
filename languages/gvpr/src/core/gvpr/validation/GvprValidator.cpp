#include "gvpr/validation/GvprValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "gvpr/CoreServices.hpp"

namespace gvpr {

void register_validation_checks(GvprCoreServices& services) {
  services.validator = std::make_unique<const GvprValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace gvpr
