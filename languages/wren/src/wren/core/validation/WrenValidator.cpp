#include "wren/core/validation/WrenValidator.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "wren/core/CoreServices.hpp"

namespace wren {

void WrenValidator::checkAssignment(const ast::AssignStat& node,
                                    cursiv::ValidationAcceptor& accept) const {
  if (!node.target) return;
  const auto* kind = node.target->type;
  const bool name = kind == &cursiv::ast_type_tag<ast::NameExpr>;
  if (name) {
    const auto text = static_cast<const ast::NameExpr&>(*node.target).name;
    if (text == "this" || text == "super" || (node.isVar && text.starts_with("_")))
      accept.error(node, "Expected an assignable variable name", "wren.assignment")
          .property<&ast::AssignStat::target>();
  }
  if ((node.isVar && !name) || (!name && kind != &cursiv::ast_type_tag<ast::FieldExpr> &&
                              kind != &cursiv::ast_type_tag<ast::IndexExpr>))
    accept.error(node, "Expected an assignable variable, property or subscript", "wren.assignment")
        .property<&ast::AssignStat::target>();
}

void WrenValidator::checkMember(const ast::Member& node,
                                cursiv::ValidationAcceptor& accept) const {
  const ast::Block* body = nullptr;
  std::vector<std::string_view> params;
  const auto collect = [&](const auto& member) {
    body = member.body;
    if constexpr (requires { member.params; })
      params.assign(member.params.begin(), member.params.end());
    if constexpr (requires { member.param; }) params.push_back(member.param);
    if constexpr (requires { member.valueParam; }) params.push_back(member.valueParam);
  };
  if (node.type == &cursiv::ast_type_tag<ast::ConstructorDef>) {
    collect(static_cast<const ast::ConstructorDef&>(node));
    if (node.isStatic) accept.error(node, "A constructor cannot be static", "wren.constructor");
  } else if (node.type == &cursiv::ast_type_tag<ast::MethodDef>) collect(static_cast<const ast::MethodDef&>(node));
  else if (node.type == &cursiv::ast_type_tag<ast::GetterDef>) collect(static_cast<const ast::GetterDef&>(node));
  else if (node.type == &cursiv::ast_type_tag<ast::SetterDef>) collect(static_cast<const ast::SetterDef&>(node));
  else if (node.type == &cursiv::ast_type_tag<ast::SubscriptGetDef>) collect(static_cast<const ast::SubscriptGetDef&>(node));
  else if (node.type == &cursiv::ast_type_tag<ast::SubscriptSetDef>) collect(static_cast<const ast::SubscriptSetDef&>(node));
  if (node.isForeign && body) accept.error(node, "A foreign method cannot have a body", "wren.body");
  if (!node.isForeign && !body) accept.error(node, "A method needs a body", "wren.body");
  if (params.size() > 16) accept.error(node, "A method accepts at most 16 parameters", "wren.parameters");
  std::unordered_set<std::string_view> names;
  for (auto name : params)
    if (!names.insert(name).second)
      accept.error(node, "Duplicate parameter name", "wren.parameters");
}

void register_validation_checks(WrenCoreServices& services) {
  services.validator = std::make_unique<const WrenValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&WrenValidator::checkMember>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&WrenValidator::checkAssignment>(*services.validator)});
}

}  // namespace wren
