#include "lpc/core/validation/LpcValidator.hpp"

#include <memory>
#include <algorithm>

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
bool async_context(const cursiv::AstNode& node) {
  for (auto* p = node.parent; p; p = p->parent) {
    if (cursiv::ast_cast<ast::CatchExpr>(p) || cursiv::ast_cast<ast::TimeExpr>(p)) return false;
    if (const auto* function = cursiv::ast_cast<ast::FuncDecl>(p))
      return function->returnType && std::find(function->returnType->modifiers.begin(), function->returnType->modifiers.end(), "async") != function->returnType->modifiers.end();
    if (cursiv::ast_cast<ast::FuncLitExpr>(p) || cursiv::ast_cast<ast::ClosureExpr>(p)) return false;
  }
  return false;
}
} // namespace
void LpcValidator::checkPrefix(const ast::PrefixExpr& node, cursiv::ValidationAcceptor& accept) const {
  if (node.op == "await" && !async_context(node)) accept.error(node, "await requires an async function outside catch/time_expression.", "lpc.async-context");
}
void LpcValidator::checkAcatch(const ast::AcatchExpr& node, cursiv::ValidationAcceptor& accept) const {
  if (!async_context(node)) accept.error(node, "acatch requires an async function outside catch/time_expression.", "lpc.async-context");
}

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
          &LpcValidator::checkCase>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&LpcValidator::checkPrefix>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&LpcValidator::checkAcatch>(*services.validator)});
}

} // namespace lpc
