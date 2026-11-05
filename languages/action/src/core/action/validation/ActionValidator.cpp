#include "action/validation/ActionValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "action/CoreServices.hpp"

namespace action {

void register_validation_checks(ActionCoreServices& services) {
  services.validator = std::make_unique<const ActionValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace action
