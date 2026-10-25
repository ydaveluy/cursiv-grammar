#include "ctl/core/validation/CtlValidator.hpp"

#include <memory>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "ctl/core/CoreServices.hpp"

namespace ctl {
namespace {
const ast::Proposition* ungroup(const ast::Proposition* node) {
  while (const auto* group = cursiv::ast_cast<ast::Grouped>(node)) node = group->inner;
  return node;
}
bool path(const ast::Proposition* node) {
  node = ungroup(node);
  return cursiv::ast_cast<ast::UnaryTemporal>(node) || cursiv::ast_cast<ast::BinaryTemporal>(node);
}
bool state(const ast::Proposition* node) { return ungroup(node) && !path(node); }
}  // namespace

void CtlValidator::checkFormula(const ast::Proposition& node,
                                cursiv::ValidationAcceptor& accept) const {
  if (cursiv::ast_cast<ast::Missing>(&node))
    accept.error(node, "Missing operand.", "ctl.operand");
  if (const auto* atom = cursiv::ast_cast<ast::AtomicProp>(&node)) {
    if (!atom->quoted && (atom->name == "A" || atom->name == "E" || atom->name == "not"))
      accept.error(*atom, "Missing operand after the operator.", "ctl.operand")
          .property<&ast::AtomicProp::name>();
  }
  if (!cursiv::ast_cast<ast::Proposition>(node.parent) && path(&node))
    accept.error(node, "A temporal formula requires an A or E path quantifier.", "ctl.quantifier");
  if (const auto* quantified = cursiv::ast_cast<ast::Quantified>(&node)) {
    if (!path(quantified->path) && !cursiv::ast_cast<ast::Missing>(ungroup(quantified->path)))
      accept.error(*quantified, "A or E must quantify a temporal formula.", "ctl.path")
          .property<&ast::Quantified::path>();
  }
  const auto checkState = [&]<class Node, auto Member>(const Node& owner) {
    if (!state(owner.*Member))
      accept.error(owner, "Expected a state formula; add an A or E path quantifier.", "ctl.state")
          .template property<Member>();
  };
  if (const auto* unary = cursiv::ast_cast<ast::UnaryTemporal>(&node))
    checkState.operator()<ast::UnaryTemporal, &ast::UnaryTemporal::operand>(*unary);
  if (const auto* binary = cursiv::ast_cast<ast::BinaryTemporal>(&node)) {
    checkState.operator()<ast::BinaryTemporal, &ast::BinaryTemporal::left>(*binary);
    checkState.operator()<ast::BinaryTemporal, &ast::BinaryTemporal::right>(*binary);
  }
  if (const auto* negated = cursiv::ast_cast<ast::Not>(&node))
    checkState.operator()<ast::Not, &ast::Not::operand>(*negated);
  if (const auto* binary = cursiv::ast_cast<ast::BinaryProp>(&node)) {
    checkState.operator()<ast::BinaryProp, &ast::BinaryProp::left>(*binary);
    checkState.operator()<ast::BinaryProp, &ast::BinaryProp::right>(*binary);
  }
}

void register_validation_checks(CtlCoreServices& services) {
  services.validator = std::make_unique<const CtlValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&CtlValidator::checkFormula>(*services.validator)});
}
}  // namespace ctl
