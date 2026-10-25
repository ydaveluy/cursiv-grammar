#include "lua/core/validation/LuaValidator.hpp"

#include <memory>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lua/core/CoreServices.hpp"

namespace lua {

void LuaValidator::checkAssignTargets(const ast::AssignStat& stat,
                                      cursiv::ValidationAcceptor& accept) const {
  for (const ast::Expr* const target : stat.targets) {
    if (target != nullptr &&
        !target->type->derivesFrom(&cursiv::ast_type_tag<ast::Var>))
      accept.error(*target, "cannot assign to this expression");
  }
}

void LuaValidator::checkCallStat(const ast::CallStat& stat,
                                 cursiv::ValidationAcceptor& accept) const {
  if (stat.call != nullptr &&
      !stat.call->type->derivesFrom(&cursiv::ast_type_tag<ast::Call>))
    accept.error(*stat.call, "syntax error: function call expected");
}

void register_validation_checks(LuaCoreServices& services) {
  using V = LuaValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssignTargets>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCallStat>(
           validator)});
}

}  // namespace lua
