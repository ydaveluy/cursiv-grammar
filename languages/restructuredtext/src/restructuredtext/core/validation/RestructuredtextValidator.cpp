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

void RestructuredtextValidator::checkHyperlinkHasUrl(
    const ast::Hyperlink& hyperlink, cursiv::ValidationAcceptor& accept) const {
  if (hyperlink.text.find('<') == std::string_view::npos)
    accept.warning(hyperlink, "Hyperlink reference has no <url>.")
        .property<&ast::Hyperlink::text>();
}

void register_validation_checks(RestructuredtextCoreServices& services) {
  using V = RestructuredtextValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkLiteralNotEmpty>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkHyperlinkHasUrl>(validator)});
}

}  // namespace restructuredtext
