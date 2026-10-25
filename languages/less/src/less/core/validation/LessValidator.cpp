#include "less/core/validation/LessValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "less/core/CoreServices.hpp"

namespace less {

void LessValidator::checkVariableDecl(
    const ast::VariableDecl& node, cursiv::ValidationAcceptor& accept) const {
  // Indirection is valid in a value and in a mixin parameter, but an ordinary
  // variable declaration needs a literal name in the native Less parser.
  if (node.name && node.name->inner &&
      !cursiv::ast_cast<ast::MixinDef>(node.parent))
    accept.error(node, "A variable declaration cannot use an indirect name",
                 "less.indirect-declaration")
        .property<&ast::VariableDecl::name>();
}

void register_validation_checks(LessCoreServices& services) {
  services.validator = std::make_unique<const LessValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<
          &LessValidator::checkVariableDecl>(*services.validator)});
}

}  // namespace less
