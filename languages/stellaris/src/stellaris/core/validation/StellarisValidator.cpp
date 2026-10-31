#include "stellaris/core/validation/StellarisValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "stellaris/core/CoreServices.hpp"

namespace stellaris {
void StellarisValidator::checkGroup(
    const ast::Group& node, cursiv::ValidationAcceptor& accept) const {
  if (!node.closed)
    accept.error(node, "Expected a closing brace.", "stellaris.unclosed-group");
}
void StellarisValidator::checkOperator(
    const ast::KeyValue& node, cursiv::ValidationAcceptor& accept) const {
  const auto op = node.op;
  if (!op.empty() && op != "=" && op != "<" && op != ">" && op != "<=" &&
      op != ">=" && op != "!=" && op != "?=" && op != "==")
    accept.error(node, "Unknown comparison operator.", "stellaris.operator")
        .property<&ast::KeyValue::op>();
}
void StellarisValidator::checkDocument(
    const ast::Document& node, cursiv::ValidationAcceptor& accept) const {
  for (const auto& item : node.items)
    if (!cursiv::ast_cast<ast::KeyValue>(item) &&
        !cursiv::ast_cast<ast::ParameterDefinition>(item))
      accept.error(node, "Expected an object entry.", "stellaris.document-item");
}
void register_validation_checks(StellarisCoreServices& services) {
  services.validator = std::make_unique<const StellarisValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&StellarisValidator::checkGroup>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&StellarisValidator::checkOperator>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&StellarisValidator::checkDocument>(validator)});
}
}  // namespace stellaris
