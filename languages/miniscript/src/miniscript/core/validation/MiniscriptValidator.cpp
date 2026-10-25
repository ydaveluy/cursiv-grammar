#include "miniscript/core/validation/MiniscriptValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "miniscript/core/CoreServices.hpp"

namespace miniscript {

void MiniscriptValidator::checkMultisig(
    const ast::MultisigExpression& expr,
    cursiv::ValidationAcceptor& accept) const {
  if (!expr.isMultiA && expr.keys.size() > 20)
    accept.error(expr, "multi permits at most 20 keys.");
}

void register_validation_checks(MiniscriptCoreServices& services) {
  services.validator = std::make_unique<const MiniscriptValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &MiniscriptValidator::checkMultisig>(validator)});
}

}  // namespace miniscript
