#include "b/core/validation/BValidator.hpp"
#include "b/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include <memory>

namespace b {
namespace {
bool lvalue(const ast::Expr *expr) {
  if (cursiv::ast_cast<ast::Ident>(expr) ||
      cursiv::ast_cast<ast::IndexExpr>(expr))
    return true;
  const auto *prefix = cursiv::ast_cast<ast::PrefixExpr>(expr);
  return prefix && prefix->op == "*";
}
bool sizeConstant(const ast::Expr *expr) {
  return !expr || cursiv::ast_cast<ast::IntLit>(expr) ||
         cursiv::ast_cast<ast::CharLit>(expr);
}
} // namespace
void BValidator::checkAssignment(const ast::AssignExpr &expr,
                                 cursiv::ValidationAcceptor &accept) const {
  if (!lvalue(expr.target))
    accept.error(expr, "The assignment target must be an lvalue.", "b.lvalue")
        .property<&ast::AssignExpr::target>();
}
void BValidator::checkPrefix(const ast::PrefixExpr &expr,
                             cursiv::ValidationAcceptor &accept) const {
  if ((expr.op == "++" || expr.op == "--") && !lvalue(expr.operand))
    accept.error(expr, "Increment and decrement require an lvalue.", "b.lvalue")
        .property<&ast::PrefixExpr::operand>();
}
void BValidator::checkPostfix(const ast::PostfixExpr &expr,
                              cursiv::ValidationAcceptor &accept) const {
  if (!lvalue(expr.operand))
    accept.error(expr, "Increment and decrement require an lvalue.", "b.lvalue")
        .property<&ast::PostfixExpr::operand>();
}
void BValidator::checkAddress(const ast::AddrExpr &expr,
                              cursiv::ValidationAcceptor &accept) const {
  if (!lvalue(expr.target))
    accept.error(expr, "Taking an address requires an lvalue.", "b.lvalue")
        .property<&ast::AddrExpr::target>();
}
void BValidator::checkVariable(const ast::VarDef &variable,
                               cursiv::ValidationAcceptor &accept) const {
  if (!sizeConstant(variable.size))
    accept.error(variable,
                 "A vector size must be an integer or character constant.",
                 "b.vector-size");
  for (std::size_t i = 1; i < variable.inits.size(); ++i) {
    const auto previous = accept.spanOf(*variable.inits[i - 1]);
    const auto next = accept.spanOf(*variable.inits[i]);
    bool comma = false;
    for (std::size_t j = 0; j < variable.inits.size(); ++j) {
      const auto span = accept.keywordSpan(variable, ",", j);
      if (!span)
        break;
      comma |= span->begin >= previous.end && span->end <= next.begin;
    }
    if (!comma)
      accept.error(*variable.inits[i], "Separate initializers with a comma.",
                   "b.initializer-separator");
  }
}
void BValidator::checkAuto(const ast::AutoVar &variable,
                           cursiv::ValidationAcceptor &accept) const {
  if (!sizeConstant(variable.size))
    accept.error(variable,
                 "A vector size must be an integer or character constant.",
                 "b.vector-size");
}
void BValidator::checkIdentifier(const ast::Ident &name,
                                 cursiv::ValidationAcceptor &accept) const {
  for (const auto keyword : {"auto", "extrn", "goto", "return", "if", "while",
                             "else", "switch", "case"})
    if (name.name == keyword)
      accept.error(name, "A keyword cannot be used as a name.", "b.keyword");
}
void BValidator::checkCase(const ast::CaseLabel &label,
                           cursiv::ValidationAcceptor &accept) const {
  for (const auto *node = label.parent; node; node = node->parent)
    if (cursiv::ast_cast<ast::SwitchStmt>(node))
      return;
  accept.error(label, "A case label must be inside a switch.",
               "b.case-context");
}
void register_validation_checks(BCoreServices &services) {
  services.validator = std::make_unique<const BValidator>(services);
  using V = BValidator;
  const auto &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssignment>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPrefix>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPostfix>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAddress>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkVariable>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAuto>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkIdentifier>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCase>(
           validator)});
}
} // namespace b
