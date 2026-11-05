#include "clif/validation/ClifValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "clif/CoreServices.hpp"

namespace clif {

void register_validation_checks(ClifCoreServices& services) {
  services.validator = std::make_unique<const ClifValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace clif
