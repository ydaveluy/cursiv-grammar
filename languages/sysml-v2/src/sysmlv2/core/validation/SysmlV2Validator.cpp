#include "sysmlv2/core/validation/SysmlV2Validator.hpp"

#include <memory>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sysmlv2/core/CoreServices.hpp"

namespace sysmlv2 {

void SysmlV2Validator::checkPostfix(const ast::Postfix& node,
                                  cursiv::ValidationAcceptor& accept) const {
  if ((node.op == "[" || node.op == "#") && node.args.empty())
    accept.error(node, "An index or bracket expression needs a sequence.", "sysml.sequence");
  bool named = false, positional = false;
  for (const auto* argument : node.args) {
    named |= cursiv::ast_cast<ast::NamedArgument>(argument) != nullptr;
    positional |= cursiv::ast_cast<ast::NamedArgument>(argument) == nullptr;
  }
  if (named && positional)
    accept.error(node, "Use either named arguments or positional arguments.", "sysml.arguments");
}

void SysmlV2Validator::checkSpec(const ast::Spec& node,
                               cursiv::ValidationAcceptor& accept) const {
  if (node.isConjugated && node.kind != ":" && node.kind != "typedby" && node.kind != "definedby")
    accept.error(node, "Conjugated port typing requires a typing relationship.", "sysml.conjugated-typing");
}

void SysmlV2Validator::checkElement(const ast::Element& node,
                                  cursiv::ValidationAcceptor& accept) const {
  if (node.isParallel && node.kind != "state")
    accept.error(node, "Only a state body may be parallel.", "sysml.parallel-state");
}

void SysmlV2Validator::checkBinary(const ast::Binary& node,
                                 cursiv::ValidationAcceptor& accept) const {
  if (node.op != "istype" && node.op != "hastype" && node.op != "@" && node.op != "@@" &&
      node.op != "as" && node.op != "meta") return;
  if (!cursiv::ast_cast<ast::NameRef>(node.right))
    accept.error(node, "A classification or cast requires a type reference.", "sysml.type-reference");
}

void register_validation_checks(SysmlV2CoreServices& services) {
  services.validator = std::make_unique<const SysmlV2Validator>(services);
  using V = SysmlV2Validator;
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkPostfix>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkSpec>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkElement>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkBinary>(validator)});
}

}  // namespace sysmlv2
