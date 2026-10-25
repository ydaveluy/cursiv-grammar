#include "geekcode/core/validation/GeekcodeValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "geekcode/core/CoreServices.hpp"

namespace geekcode {

void GeekcodeValidator::checkTuple(const ast::Tuple& tuple,
                                 cursiv::ValidationAcceptor& accept) const {
  const auto* category = std::get_if<std::string_view>(&tuple.kind);
  for (const auto& modifier : tuple.modifiers) {
    const auto* text = std::get_if<std::string_view>(&modifier);
    if (!text) continue;
    if ((*text == "pu" || *text == "x") && (!category || *category != "d"))
      accept.error(tuple, "The x and pu modifiers apply to dress (d).",
                   "geekcode.modifier.dress").property<&ast::Tuple::modifiers>();
    if (*text == "%" && (!category || *category != "r"))
      accept.error(tuple, "The % modifier applies to relationships (r).",
                   "geekcode.modifier.relationships").property<&ast::Tuple::modifiers>();
  }
}

void GeekcodeValidator::checkShape(const ast::ShapeSegment& segment,
                                 cursiv::ValidationAcceptor& accept) const {
  for (const auto& modifier : segment.modifiers) {
    const auto* text = std::get_if<std::string_view>(&modifier);
    if (text && (*text == "pu" || *text == "x" || *text == "%"))
      accept.error(segment, "Shape uses + and - ratings.",
                   "geekcode.modifier.shape").property<&ast::ShapeSegment::modifiers>();
  }
}

void register_validation_checks(GeekcodeCoreServices& services) {
  services.validator = std::make_unique<const GeekcodeValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&GeekcodeValidator::checkTuple>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&GeekcodeValidator::checkShape>(*services.validator)});
}

}  // namespace geekcode
