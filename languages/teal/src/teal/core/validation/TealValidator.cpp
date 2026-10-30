#include "teal/core/validation/TealValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "teal/core/CoreServices.hpp"

namespace teal {

void TealValidator::checkNewType(const ast::NewType& node,
                                 cursiv::ValidationAcceptor& accept) const {
  if (node.legacy)
    accept.error(node, "Use a named record/enum declaration or a type alias.", "teal.legacy-type");
}
void register_validation_checks(TealCoreServices& services) {
  services.validator = std::make_unique<const TealValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&TealValidator::checkNewType>(*services.validator)});
}

}  // namespace teal
