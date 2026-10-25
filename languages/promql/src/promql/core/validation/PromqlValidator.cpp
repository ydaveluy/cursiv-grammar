#include "promql/core/validation/PromqlValidator.hpp"

#include <charconv>
#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "promql/core/CoreServices.hpp"

namespace promql {
namespace {

bool duration_expression(const ast::Expression* expression) {
  if (!expression) return true;
  if (cursiv::ast_cast<ast::NumberLit>(expression) ||
      cursiv::ast_cast<ast::DurationLit>(expression)) return true;
  if (const auto* unary = cursiv::ast_cast<ast::UnaryExpr>(expression))
    return duration_expression(unary->operand);
  if (const auto* binary = cursiv::ast_cast<ast::BinaryExpr>(expression)) {
    switch (binary->op) {
      case ast::Operator::add: case ast::Operator::subtract:
      case ast::Operator::multiply: case ast::Operator::divide:
      case ast::Operator::modulo: case ast::Operator::pow:
        return duration_expression(binary->left) && duration_expression(binary->right);
      default: return false;
    }
  }
  if (const auto* call = cursiv::ast_cast<ast::FunctionCall>(expression)) {
    auto name = std::string(call->name);
    for (auto& c : name)
      if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    if (name == "step" || name == "range") return call->args.empty();
    if (name == "min_of" || name == "max_of")
      return call->args.size() == 2 && duration_expression(call->args[0]) &&
             duration_expression(call->args[1]);
  }
  return false;
}

}  // namespace

void PromqlValidator::checkSelectorDurations(
    const ast::Selector& node, cursiv::ValidationAcceptor& accept) const {
  if (!duration_expression(node.range))
    accept.error(node, "A range must be a duration expression.")
        .property<&ast::Selector::range>();
  if (!duration_expression(node.offset))
    accept.error(node, "An offset must be a duration expression.")
        .property<&ast::Selector::offset>();
}

void PromqlValidator::checkSubqueryDurations(
    const ast::Subquery& node, cursiv::ValidationAcceptor& accept) const {
  if (!duration_expression(node.range))
    accept.error(node, "A range must be a duration expression.")
        .property<&ast::Subquery::range>();
  if (!duration_expression(node.step))
    accept.error(node, "A step must be a duration expression.")
        .property<&ast::Subquery::step>();
  if (!duration_expression(node.offset))
    accept.error(node, "An offset must be a duration expression.")
        .property<&ast::Subquery::offset>();
}

void PromqlValidator::checkTimestampTarget(
    const ast::Timestamped& node, cursiv::ValidationAcceptor& accept) const {
  const ast::Expression* operand = node.operand;
  while (const auto* modified = cursiv::ast_cast<ast::RangeModifier>(operand))
    operand = modified->operand;
  if (operand && operand->type != &cursiv::ast_type_tag<ast::Selector> &&
      operand->type != &cursiv::ast_type_tag<ast::Subquery>)
    accept.error(node,
                 "The @ modifier applies to a vector selector or subquery.");
  const auto* unary = cursiv::ast_cast<ast::UnaryExpr>(node.timestamp);
  if (node.timestamp &&
      node.timestamp->type != &cursiv::ast_type_tag<ast::NumberLit> &&
      node.timestamp->type != &cursiv::ast_type_tag<ast::DurationLit> &&
      node.timestamp->type != &cursiv::ast_type_tag<ast::TimestampBoundary> &&
      !(unary && (cursiv::ast_cast<ast::NumberLit>(unary->operand) ||
                  cursiv::ast_cast<ast::DurationLit>(unary->operand))))
    accept.error(node, "The @ timestamp must be a number, start() or end().")
        .property<&ast::Timestamped::timestamp>();
  if (!duration_expression(node.offset))
    accept.error(node, "An offset must be a duration expression.")
        .property<&ast::Timestamped::offset>();
}

void register_validation_checks(PromqlCoreServices& services) {
  using V = PromqlValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkSelectorDurations>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkSubqueryDurations>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
          &PromqlValidator::checkTimestampTarget>(*services.validator)});
}

}  // namespace promql
