#include "tnt/core/validation/TntValidator.hpp"

#include <memory>
#include <set>
#include <string_view>
#include <utility>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tnt/core/CoreServices.hpp"

namespace tnt {
namespace {
using Variable = std::pair<std::string_view, std::uint64_t>;
using Variables = std::set<Variable>;
void term_variables(const ast::Term* term, Variables& variables) {
  if (const auto* variable = cursiv::ast_cast<ast::Variable>(term))
    variables.emplace(variable->letter, variable->primes);
  else if (const auto* successor = cursiv::ast_cast<ast::SuccessorExpression>(term))
    term_variables(successor->expression, variables);
  else if (const auto* group = cursiv::ast_cast<ast::GroupedExpression>(term))
    term_variables(group->expression, variables);
  else if (const auto* binary = cursiv::ast_cast<ast::BinaryExpression>(term)) {
    term_variables(binary->left, variables);
    term_variables(binary->right, variables);
  }
}
struct Scope { Variables free, quantified; };
Scope scope(const ast::Formula* formula) {
  if (const auto* equation = cursiv::ast_cast<ast::Equation>(formula)) {
    Scope result;
    term_variables(equation->left, result.free);
    term_variables(equation->right, result.free);
    return result;
  }
  if (const auto* negated = cursiv::ast_cast<ast::Negation>(formula)) return scope(negated->expression);
  if (const auto* quantified = cursiv::ast_cast<ast::QuantifiedExpression>(formula)) {
    auto result = scope(quantified->expression);
    if (quantified->variable) {
      const Variable variable{quantified->variable->letter, quantified->variable->primes};
      result.free.erase(variable);
      result.quantified.insert(variable);
    }
    return result;
  }
  if (const auto* binary = cursiv::ast_cast<ast::LogicalExpression>(formula)) {
    auto result = scope(binary->left), right = scope(binary->right);
    result.free.insert(right.free.begin(), right.free.end());
    result.quantified.insert(right.quantified.begin(), right.quantified.end());
    return result;
  }
  return {};
}
}  // namespace

void TntValidator::checkTerm(const ast::Term& node,
                             cursiv::ValidationAcceptor& accept) const {
  if (const auto* variable = cursiv::ast_cast<ast::Variable>(&node)) {
    if (variable->letter.size() != 1 || variable->letter[0] < 'a' || variable->letter[0] > 'e')
      accept.error(*variable, "A TNT variable uses a, b, c, d or e.", "tnt.variable")
          .property<&ast::Variable::letter>();
  }
  if (const auto* binary = cursiv::ast_cast<ast::BinaryExpression>(&node)) {
    if (!cursiv::ast_cast<ast::GroupedExpression>(node.parent))
      accept.error(*binary, "Enclose the arithmetic operation in parentheses.", "tnt.term")
          .property<&ast::BinaryExpression::op>();
  }
  if (const auto* group = cursiv::ast_cast<ast::GroupedExpression>(&node)) {
    if (group->expression && !cursiv::ast_cast<ast::BinaryExpression>(group->expression))
      accept.error(*group, "A parenthesized TNT term requires + or multiplication.", "tnt.term");
  }
}

void TntValidator::checkCompound(const ast::LogicalExpression& node,
                                 cursiv::ValidationAcceptor& accept) const {
  const auto left = scope(node.left), right = scope(node.right);
  for (const auto& variable : left.free) if (right.quantified.contains(variable)) {
    accept.error(node, "A variable is free on the left and quantified on the right.", "tnt.binding")
        .property<&ast::LogicalExpression::right>();
    break;
  }
  for (const auto& variable : right.free) if (left.quantified.contains(variable)) {
    accept.error(node, "A variable is quantified on the left and free on the right.", "tnt.binding")
        .property<&ast::LogicalExpression::left>();
    break;
  }
}

void register_validation_checks(TntCoreServices& services) {
  services.validator = std::make_unique<const TntValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&TntValidator::checkTerm>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&TntValidator::checkCompound>(*services.validator)});
}
}  // namespace tnt
