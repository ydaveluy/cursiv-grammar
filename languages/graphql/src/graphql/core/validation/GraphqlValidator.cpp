#include "graphql/core/validation/GraphqlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "graphql/core/CoreServices.hpp"

namespace graphql {

void GraphqlValidator::checkDefinition(
    const ast::Definition& node, cursiv::ValidationAcceptor& accept) const {
  bool empty = false;
  if (const auto* n = cursiv::ast_cast<ast::SchemaExtension>(&node))
    empty = n->directives.empty() && n->operationTypes.empty();
  else if (const auto* n = cursiv::ast_cast<ast::ScalarTypeExtension>(&node))
    empty = n->directives.empty();
  else if (const auto* n = cursiv::ast_cast<ast::ObjectTypeExtension>(&node))
    empty = n->directives.empty() && n->interfaces.empty() && n->fields.empty();
  else if (const auto* n = cursiv::ast_cast<ast::InterfaceTypeExtension>(&node))
    empty = n->directives.empty() && n->interfaces.empty() && n->fields.empty();
  else if (const auto* n = cursiv::ast_cast<ast::UnionTypeExtension>(&node))
    empty = n->directives.empty() && n->members.empty();
  else if (const auto* n = cursiv::ast_cast<ast::EnumTypeExtension>(&node))
    empty = n->directives.empty() && n->values.empty();
  else if (const auto* n = cursiv::ast_cast<ast::InputObjectTypeExtension>(&node))
    empty = n->directives.empty() && n->fields.empty();
  if (empty)
    accept.error(node, "An extension must add directives or a body",
                 "graphql.empty-extension");
  if (const auto* n = cursiv::ast_cast<ast::FragmentDefinition>(&node);
      n && n->name == "on")
    accept.error(*n, "A fragment cannot be named on", "graphql.fragment-name")
        .property<&ast::FragmentDefinition::name>();
}

void GraphqlValidator::checkVariable(
    const ast::Variable& node, cursiv::ValidationAcceptor& accept) const {
  for (const auto* parent = node.parent; parent; parent = parent->parent)
    if (cursiv::ast_cast<ast::VariableDefinition>(parent) ||
        cursiv::ast_cast<ast::InputValueDefinition>(parent) ||
        cursiv::ast_cast<ast::TypeDefinition>(parent) ||
        cursiv::ast_cast<ast::TypeExtension>(parent) ||
        cursiv::ast_cast<ast::SchemaDefinition>(parent) ||
        cursiv::ast_cast<ast::SchemaExtension>(parent) ||
        cursiv::ast_cast<ast::DirectiveDefinition>(parent)) {
      accept.error(node, "Variables are not allowed in a constant value",
                   "graphql.constant-value")
          .property<&ast::Variable::name>();
      return;
    }
}

void GraphqlValidator::checkFragmentSpread(
    const ast::FragmentSpread& node, cursiv::ValidationAcceptor& accept) const {
  if (node.name == "on")
    accept.error(node, "A fragment cannot be named on", "graphql.fragment-name")
        .property<&ast::FragmentSpread::name>();
}

void GraphqlValidator::checkEnumValue(
    const ast::EnumValueDefinition& node,
    cursiv::ValidationAcceptor& accept) const {
  if (node.name == "true" || node.name == "false" || node.name == "null")
    accept.error(node, "An enum value cannot be true, false or null",
                 "graphql.enum-value")
        .property<&ast::EnumValueDefinition::name>();
}

void register_validation_checks(GraphqlCoreServices& services) {
  services.validator = std::make_unique<const GraphqlValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<
          &GraphqlValidator::checkDefinition>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<
          &GraphqlValidator::checkVariable>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<
          &GraphqlValidator::checkFragmentSpread>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<
          &GraphqlValidator::checkEnumValue>(*services.validator)});
}

}  // namespace graphql
