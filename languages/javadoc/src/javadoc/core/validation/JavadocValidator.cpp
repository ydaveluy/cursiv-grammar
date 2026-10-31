#include "javadoc/core/validation/JavadocValidator.hpp"

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "javadoc/core/Content.hpp"
#include "javadoc/core/CoreServices.hpp"
#include <memory>

namespace javadoc {

void JavadocValidator::checkInline(const ast::InlineTag &tag,
                                   cursiv::ValidationAcceptor &accept) const {
  if (tag.name.starts_with('_'))
    accept.error(tag, "A Javadoc tag name must start with a Unicode letter.")
        .property<&ast::InlineTag::name>()
        .code("javadoc.tag-name");
  const auto content = inline_content(tag);
  if ((tag.name == "docRoot" || tag.name == "inheritDoc") &&
      content.find_first_not_of(" \t\r\n\f") != content.npos)
    accept.error(tag, "This inline tag does not accept content.")
        .property<&ast::InlineTag::content>()
        .code("javadoc.unexpected-content");
  if ((tag.name == "index" || tag.name == "systemProperty") && content.empty()) {
    auto finding = accept.error(tag, "This inline tag requires content.");
    finding.code("javadoc.missing-content");
    if (tag.structured) finding.property<&ast::InlineTag::name>();
    else finding.property<&ast::InlineTag::content>();
  }
  if ((tag.name == "link" || tag.name == "linkplain") && content.empty())
    accept.warning(tag, "No reference given for this link.")
        .property<&ast::InlineTag::name>()
        .code("javadoc.missing-reference");
}

void JavadocValidator::checkBlock(const ast::BlockTag &tag,
                                  cursiv::ValidationAcceptor &accept) const {
  if (tag.name.starts_with('_'))
    accept.error(tag, "A Javadoc tag name must start with a Unicode letter.")
        .property<&ast::BlockTag::name>()
        .code("javadoc.tag-name");
  for (const auto name :
       {"code", "docRoot", "index", "inheritDoc", "link", "linkplain",
        "literal", "snippet", "summary", "systemProperty", "value"})
    if (tag.name == name) {
      accept.error(tag, "This Javadoc tag uses the inline form '{@...}'.")
          .property<&ast::BlockTag::name>()
          .code("javadoc.inline-tag");
      break;
    }
}

void register_validation_checks(JavadocCoreServices &services) {
  using V = JavadocValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkInline>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBlock>(
           *services.validator)});
}

} // namespace javadoc
