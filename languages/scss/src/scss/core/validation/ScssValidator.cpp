#include "scss/core/validation/ScssValidator.hpp"

#include <charconv>
#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "scss/core/CoreServices.hpp"

namespace scss {

void ScssValidator::checkReturnContext(
    const ast::ReturnStmt& node, cursiv::ValidationAcceptor& accept) const {
  for (const auto* parent = node.parent; parent; parent = parent->parent)
    if (parent->type == &cursiv::ast_type_tag<ast::FunctionDeclaration>) return;
  accept.error(node, "@return is only valid inside a function.");
}

void ScssValidator::checkVariableFlags(
    const ast::VariableDeclaration& node, cursiv::ValidationAcceptor& accept) const {
  if (node.isGlobal && !node.nsPrefix.empty())
    accept.error(node, "!global is not allowed on a variable in another module.",
                 "scss.variable.global");
}

void register_validation_checks(ScssCoreServices& services) {
  services.validator = std::make_unique<const ScssValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &ScssValidator::checkVariableFlags>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &ScssValidator::checkReturnContext>(*services.validator)});
}

}  // namespace scss
