#include "xml/core/validation/XmlValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "xml/core/CoreServices.hpp"

namespace xml {

void XmlValidator::checkCloseTagMatches(
    const ast::Element& element, cursiv::ValidationAcceptor& accept) const {
  if (element.closeName.empty() || element.closeName == element.name) return;
  const std::string message = "Closing tag </" +
                              std::string(element.closeName) +
                              "> does not match <" +
                              std::string(element.name) + ">.";
  accept.error(element, message).property<&ast::Element::closeName>();
}

void register_validation_checks(XmlCoreServices& services) {
  using V = XmlValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkCloseTagMatches>(validator)});
}

}  // namespace xml
