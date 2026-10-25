#include "lpc/core/validation/LpcValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lpc/core/CoreServices.hpp"

namespace lpc {

namespace {
// Name resolution and constant folding can refine this check later. Mutation
// is already unambiguously invalid in a constant-expression label.
bool mutates(const ast::Expr *expression) {
  if (!expression)
    return false;
  if (cursiv::ast_cast<ast::AssignExpr>(expression))
    return true;
  if (const auto *e = cursiv::ast_cast<ast::PrefixExpr>(expression))
    return e->op == "++" || e->op == "--" || mutates(e->operand);
  if (const auto *e = cursiv::ast_cast<ast::PostfixExpr>(expression))
    return e->op == "++" || e->op == "--" || mutates(e->operand);
  if (const auto *e = cursiv::ast_cast<ast::BinaryExpr>(expression))
    return mutates(e->left) || mutates(e->right);
  if (const auto *e = cursiv::ast_cast<ast::CondExpr>(expression))
    return mutates(e->cond) || mutates(e->thenValue) || mutates(e->elseValue);
  if (const auto *e = cursiv::ast_cast<ast::CastExpr>(expression))
    return mutates(e->operand);
  return false;
}
} // namespace

void LpcValidator::checkCase(const ast::CaseStmt &label,
                             cursiv::ValidationAcceptor &accept) const {
  if (mutates(label.low))
    accept
        .error(label, "A case label must be constant; mutation is not allowed.")
        .property<&ast::CaseStmt::low>();
  if (mutates(label.high))
    accept
        .error(label,
               "A case range bound must be constant; mutation is not allowed.")
        .property<&ast::CaseStmt::high>();
}

void register_validation_checks(LpcCoreServices &services) {
  services.validator = std::make_unique<const LpcValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &LpcValidator::checkCase>(*services.validator)});
}

} // namespace lpc
