#include "restructuredtext/core/validation/RestructuredtextValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "restructuredtext/core/CoreServices.hpp"

namespace restructuredtext {

void RestructuredtextValidator::checkLiteralNotEmpty(
    const ast::Literal& literal, cursiv::ValidationAcceptor& accept) const {
  if (literal.text.empty())
    accept.warning(literal, "Empty inline literal.")
        .property<&ast::Literal::text>();
}

void register_validation_checks(RestructuredtextCoreServices& services) {
  using V = RestructuredtextValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkLiteralNotEmpty>(validator)});
}

}  // namespace restructuredtext
