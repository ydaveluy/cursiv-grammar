#include "clif/core/validation/ClifValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "clif/core/CoreServices.hpp"

namespace clif {

void register_validation_checks(ClifCoreServices& services) {
  services.validator = std::make_unique<const ClifValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace clif
