#include "pike/validation/PikeValidator.hpp"

#include "pike/Integer.hpp"
#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "pike/CoreServices.hpp"

namespace pike {

void PikeValidator::checkNumber(const ast::NumberLit &node,
                                cursiv::ValidationAcceptor &accept) const {
  if (!integer_value(node.value))
    accept.error(node, "Invalid digit in an integer literal.", "pike.integer")
        .property<&ast::NumberLit::value>();
}

void register_validation_checks(PikeCoreServices &services) {
  services.validator = std::make_unique<const PikeValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &PikeValidator::checkNumber>(*services.validator)});
}

} // namespace pike
