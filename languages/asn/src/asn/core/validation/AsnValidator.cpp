#include "asn/core/validation/AsnValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "asn/core/CoreServices.hpp"

namespace asn {

void AsnValidator::checkTag(const ast::TaggedType& tag,
                           cursiv::ValidationAcceptor& accept) const {
  if (tag.tagClass == ast::TagClass::universal)
    accept.error(tag, "UNIVERSAL tags are reserved for standard-defined types.",
                 "asn.universal-tag").property<&ast::TaggedType::tagClass>();
  if (tag.mode == ast::TagMode::implicit_ &&
      cursiv::ast_cast<ast::ChoiceType>(tag.inner))
    accept.error(tag, "An untagged CHOICE cannot be tagged IMPLICIT.",
                 "asn.implicit-choice").property<&ast::TaggedType::mode>();
}

void AsnValidator::checkReal(const ast::Type* type, const ast::Value* value,
                            cursiv::ValidationAcceptor& accept) const {
  while (const auto* tag = cursiv::ast_cast<ast::TaggedType>(type))
    type = tag->inner;
  if (!cursiv::ast_cast<ast::RealType>(type) || !value) return;
  if (const auto* composite = cursiv::ast_cast<ast::CompositeRealValue>(value)) {
    if (const auto* mantissa = cursiv::ast_cast<ast::IntegerValue>(composite->mantissa))
      if (mantissa->text.find_first_not_of("-0") == std::string_view::npos)
        accept.error(*mantissa, "The REAL associated sequence requires a nonzero mantissa.",
                     "asn.real-mantissa");
    if (const auto* base = cursiv::ast_cast<ast::IntegerValue>(composite->base))
      if (base->text != "2" && base->text != "10")
        accept.error(*base, "The REAL base must be 2 or 10.", "asn.real-base");
  } else if (!cursiv::ast_cast<ast::IntegerValue>(value) &&
             !cursiv::ast_cast<ast::RealNumberValue>(value) &&
             !cursiv::ast_cast<ast::NameValue>(value) &&
             !cursiv::ast_cast<ast::SpecialRealValue>(value)) {
    accept.error(*value, "REAL needs a numeric/defined/special value or its mantissa, base, exponent sequence.",
                 "asn.real-value");
  }
}

void AsnValidator::checkValueAssignment(const ast::ValueAssignment& assignment,
                                       cursiv::ValidationAcceptor& accept) const {
  checkReal(assignment.type, assignment.value, accept);
}

void AsnValidator::checkComponent(const ast::Component& component,
                                 cursiv::ValidationAcceptor& accept) const {
  if (component.item) checkReal(component.item->type, component.default_, accept);
}

void AsnValidator::checkFieldSpec(const ast::FieldSpec& field,
                                 cursiv::ValidationAcceptor& accept) const {
  checkReal(field.type, field.defaultValue, accept);
}

void register_validation_checks(AsnCoreServices& services) {
  using V = AsnValidator;
  services.validator = std::make_unique<const V>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkTag>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkValueAssignment>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkComponent>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFieldSpec>(validator)});
}

}  // namespace asn
