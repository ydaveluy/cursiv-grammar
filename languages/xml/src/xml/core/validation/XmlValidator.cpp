#include "xml/core/validation/XmlValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>

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

void XmlValidator::checkUniqueAttributes(
    const ast::Element& element, cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> names;
  for (const auto* attribute : element.attributes)
    if (!names.insert(attribute->name).second)
      accept.error(*attribute, "An attribute may occur only once in a start tag.",
                   "xml.duplicate-attribute").property<&ast::Attribute::name>();
}

void XmlValidator::checkDeclaration(
    const ast::Declaration& declaration, cursiv::ValidationAcceptor& accept) const {
  // XMLDecl is version, then optional encoding, then optional standalone.
  unsigned previous = 0;
  for (const auto* attribute : declaration.attributes) {
    const unsigned position = attribute->name == "version" ? 1
                              : attribute->name == "encoding" ? 2
                              : attribute->name == "standalone" ? 3 : 0;
    if (position == 0 || position <= previous || (previous == 0 && position != 1))
      accept.error(*attribute, "Expected version, optional encoding, then optional standalone.",
                   "xml.declaration-order").property<&ast::Attribute::name>();
    previous = position;
    const auto value = attribute->value;
    bool valid = true;
    if (position == 1) {
      valid = value.starts_with("1.") && value.size() > 2;
      for (char c : value.substr(value.size() >= 2 ? 2 : value.size()))
        valid = valid && c >= '0' && c <= '9';
    } else if (position == 2) {
      const auto letter = [](char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
      };
      valid = !value.empty() && letter(value.front());
      for (char c : value)
        valid = valid && (letter(c) || (c >= '0' && c <= '9') ||
                          c == '.' || c == '_' || c == '-');
    } else if (position == 3) {
      valid = value == "yes" || value == "no";
    }
    if (!valid)
      accept.error(*attribute, "Invalid XML declaration value.",
                   "xml.declaration-value").property<&ast::Attribute::value>();
  }
}

void register_validation_checks(XmlCoreServices& services) {
  using V = XmlValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkCloseTagMatches>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkUniqueAttributes>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkDeclaration>(validator)});
}

}  // namespace xml
