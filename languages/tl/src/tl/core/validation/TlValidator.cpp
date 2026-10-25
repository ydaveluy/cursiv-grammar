#include "tl/core/validation/TlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tl/core/CoreServices.hpp"

namespace tl {

void TlValidator::checkMissing(const ast::Missing& node, cursiv::ValidationAcceptor& accept) const {
  accept.error(node, "Missing operand.", "tl.operand");
}

void register_validation_checks(TlCoreServices& services) {
  services.validator = std::make_unique<const TlValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&TlValidator::checkMissing>(*services.validator)});
}

}  // namespace tl
