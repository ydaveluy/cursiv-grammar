#include "ada/core/validation/AdaValidator.hpp"

#include <memory>

#include "ada/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace ada {

namespace {
bool logical(ast::Operator op) {
  return op == ast::Operator::and_ || op == ast::Operator::or_ ||
         op == ast::Operator::xor_ || op == ast::Operator::and_then ||
         op == ast::Operator::or_else;
}
} // namespace
void AdaValidator::checkLogicalChain(const ast::BinaryExpr &node,
                                     cursiv::ValidationAcceptor &accept) const {
  if (!logical(node.op))
    return;
  const auto *left = cursiv::ast_cast<ast::BinaryExpr>(node.left);
  if (left && logical(left->op) && left->op != node.op)
    accept
        .error(node, "Different logical operators require parentheses in Ada.")
        .property<&ast::BinaryExpr::op>();
}
void register_validation_checks(AdaCoreServices& services) {
  services.validator = std::make_unique<const AdaValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &AdaValidator::checkLogicalChain>(
          *static_cast<const AdaValidator *>(services.validator.get()))});
}

}  // namespace ada
