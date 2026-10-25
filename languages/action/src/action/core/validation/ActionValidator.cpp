#include "action/core/validation/ActionValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "action/core/CoreServices.hpp"

namespace action {

void register_validation_checks(ActionCoreServices& services) {
  services.validator = std::make_unique<const ActionValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace action
