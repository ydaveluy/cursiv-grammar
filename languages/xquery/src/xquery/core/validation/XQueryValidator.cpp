#include "xquery/core/validation/XQueryValidator.hpp"
#include <memory>
#include <unordered_set>
#include <variant>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "xquery/core/CoreServices.hpp"
#include "xquery/core/StringLiteral.hpp"

namespace xquery {
void XQueryValidator::checkElement(const ast::DirElemConstructor& element,
                                   cursiv::ValidationAcceptor& accept) const {
  if (!element.selfClosing && !element.name.empty() && !element.closeName.empty() && element.name != element.closeName)
    accept.error(element, "Opening and closing element names must agree.", "xquery.element-name")
        .property<&ast::DirElemConstructor::closeName>();
  std::unordered_set<std::string_view> names;
  for (const auto* attribute : element.attributes) {
    if (!attribute) continue;
    if (!names.insert(attribute->name).second)
      accept.error(*attribute, "Duplicate attribute name.", "xquery.attribute-name").property<&ast::DirAttr::name>();
    for (std::size_t i = 0; i < attribute->value.size(); ++i)
      if (const auto* text = std::get_if<std::string_view>(&attribute->value[i]); text && !valid_character_references(*text))
        accept.error(*attribute, "Invalid XML character reference.", "xquery.character-reference")
            .property<&ast::DirAttr::value>(i);
  }
  for (std::size_t i = 0; i < element.content.size(); ++i) {
    const auto* text = std::get_if<std::string_view>(&element.content[i]);
    if (text && !text->starts_with("<!") && !text->starts_with("<?") && !valid_character_references(*text))
      accept.error(element, "Invalid XML character reference.", "xquery.character-reference")
          .property<&ast::DirElemConstructor::content>(i);
  }
}
void register_validation_checks(XQueryCoreServices& services) {
  using V = XQueryValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkElement>(*services.validator)});
}
} // namespace xquery
