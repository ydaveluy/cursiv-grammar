#include "pddl/core/validation/PddlValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pddl/core/CoreServices.hpp"

namespace pddl {
namespace {
bool nondeterministic(std::string_view text) {
  constexpr std::string_view expected = ":non-deterministic";
  if (text.size() != expected.size()) return false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const auto c = text[i] >= 'A' && text[i] <= 'Z' ? text[i] + ('a' - 'A') : text[i];
    if (c != expected[i]) return false;
  }
  return true;
}
} // namespace

void PddlValidator::checkExpression(
    const ast::Expr& node, cursiv::ValidationAcceptor& accept) const {
  if (!node.number.empty() || node.head || !node.variable.empty() ||
      node.isDuration || node.isTotalTime || !node.violatedName.empty())
    return;
  const auto count = node.operands.size();
  const bool valid = node.arith == ast::ArithOp::Sub
                         ? count == 1 || count == 2
                     : node.arith == ast::ArithOp::Div ? count == 2
                                                      : count >= 2;
  if (!valid)
    accept.error(node, "Invalid number of arithmetic operands.",
                 "pddl.expression-arity")
        .property<&ast::Expr::arith>();
}

void PddlValidator::checkEffect(
    const ast::Effect& node, cursiv::ValidationAcceptor& accept) const {
  if (!node.isOneOf) return;
  if (node.operands.empty())
    accept.error(node, "oneof needs at least one effect.", "pddl.oneof-arity")
        .property<&ast::Effect::isOneOf>();
  for (const cursiv::AstNode* parent = node.parent; parent; parent = parent->parent)
    if (const auto* domain = cursiv::ast_cast<ast::Domain>(parent)) {
      for (auto requirement : domain->requirements)
        if (nondeterministic(requirement)) return;
      accept.error(node, "oneof requires :non-deterministic.",
                   "pddl.oneof-requirement")
          .property<&ast::Effect::isOneOf>();
      return;
    }
}

void register_validation_checks(PddlCoreServices& services) {
  services.validator = std::make_unique<const PddlValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<
          &PddlValidator::checkExpression>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<
          &PddlValidator::checkEffect>(*services.validator)});
}

}  // namespace pddl
