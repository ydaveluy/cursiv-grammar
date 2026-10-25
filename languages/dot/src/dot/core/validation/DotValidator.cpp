#include "dot/core/validation/DotValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "dot/core/CoreServices.hpp"

namespace dot {

void DotValidator::checkEdges(const ast::CompoundStmt& statement,
                              cursiv::ValidationAcceptor& accept) const {
  const cursiv::AstNode* root = &statement;
  while (root && !cursiv::ast_cast<ast::Graph>(root)) root = root->parent;
  const auto* graph = cursiv::ast_cast<ast::Graph>(root);
  if (!graph) return;
  for (const auto op : statement.ops)
    if ((op == ast::EdgeOp::directed) != graph->directed)
      accept.error(statement,
                   graph->directed ? "Use '->' for an edge in a digraph."
                                   : "Use '--' for an edge in a graph.",
                   "dot.edge-operator");
}

void DotValidator::checkAttribute(const ast::AttrPair& attribute,
                                  cursiv::ValidationAcceptor& accept) const {
  if (!attribute.value)
    accept.error(attribute, "An attribute requires '= value'.", "dot.attribute-value");
}

void register_validation_checks(DotCoreServices& services) {
  using V = DotValidator;
  services.validator = std::make_unique<const DotValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkEdges>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkAttribute>(validator)});
}

}  // namespace dot
