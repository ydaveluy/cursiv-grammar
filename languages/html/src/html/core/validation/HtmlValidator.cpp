#include "html/core/validation/HtmlValidator.hpp"

#include <memory>
#include <string>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "html/core/CoreServices.hpp"

namespace html {

void HtmlValidator::checkAttributes(const ast::Element& element,
                                    cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string> names;
  for (const auto* attribute : element.attributes) {
    if (!attribute) continue;
    std::string name(attribute->name);
    for (char& c : name) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (!names.insert(name).second)
      accept.error(*attribute, "Duplicate attribute name.", "html.duplicate-attribute")
          .property<&ast::Attribute::name>();
    if (name.find_first_of("\"'<") != name.npos)
      accept.error(*attribute, "Invalid character in attribute name.", "html.attribute-name")
          .property<&ast::Attribute::name>();
    const auto* document = attribute->getDocument();
    const auto span = accept.featureSpan(*attribute, "value", 0);
    if (!document || !span) continue;
    const auto source = document->textDocument().getText();
    const auto raw = source.substr(span->begin, span->end - span->begin);
    if (!raw.empty() && raw.front() != '\'' && raw.front() != '"' &&
        raw.find_first_of("\"'=<`") != raw.npos)
      accept.error(*attribute, "Quote this attribute value.", "html.attribute-value")
          .property<&ast::Attribute::value>();
  }
}

void HtmlValidator::checkCloseTagMatches(
    const ast::Element &element, cursiv::ValidationAcceptor &accept) const {
  if (element.name.empty() ||
      !((element.name[0] >= 'A' && element.name[0] <= 'Z') ||
        (element.name[0] >= 'a' && element.name[0] <= 'z')))
    accept.error(element, "An HTML tag name must start with an ASCII letter.",
                 "html.tag-name").property<&ast::Element::name>();
  if (!element.closeName.has_value())
    return;
  const auto lower = [](unsigned char c) {
    return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
  };
  const auto close = *element.closeName;
  bool matches = close.size() == element.name.size();
  for (std::size_t i = 0; matches && i < close.size(); ++i)
    matches = lower(close[i]) == lower(element.name[i]);
  if (!matches)
    accept.error(element, "Closing tag name does not match the opening tag.")
        .property<&ast::Element::closeName>();
}

void HtmlValidator::checkDeclarations(
    const ast::Document &document, cursiv::ValidationAcceptor &accept) const {
  if (document.doctypes.size() > 1)
    accept.error(document, "A document may have only one doctype declaration.")
        .property<&ast::Document::doctypes>();
  if (document.xmlDeclarations.size() > 1)
    accept.error(document, "A document may have only one XML declaration.")
        .property<&ast::Document::xmlDeclarations>();
}

void register_validation_checks(HtmlCoreServices &services) {
  using V = HtmlValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkCloseTagMatches>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkDeclarations>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkAttributes>(validator)});
}

} // namespace html
