#include "squirrel/core/validation/SquirrelValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "squirrel/core/CoreServices.hpp"

namespace squirrel {
namespace {
void checkParams(const auto& function, cursiv::ValidationAcceptor& accept) {
  bool sawDefault = false;
  for (const auto* param : function.params) {
    if (param->defaultValue) sawDefault = true;
    else if (sawDefault)
      accept.error(*param, "A parameter after a defaulted parameter needs a default.",
                   "squirrel.parameter-default");
  }
  if (sawDefault && function.variadic)
    accept.error(function, "Default parameters cannot be combined with varargs.",
                 "squirrel.parameter-varargs");
}
}
void SquirrelValidator::checkFunction(const ast::FunctionDecl& n, cursiv::ValidationAcceptor& a) const { checkParams(n, a); }
void SquirrelValidator::checkFunctionExpr(const ast::FunctionExpr& n, cursiv::ValidationAcceptor& a) const { checkParams(n, a); }
void SquirrelValidator::checkLambda(const ast::LambdaExpr& n, cursiv::ValidationAcceptor& a) const { checkParams(n, a); }
void SquirrelValidator::checkConstructor(const ast::ConstructorDecl& n, cursiv::ValidationAcceptor& a) const { checkParams(n, a); }


void register_validation_checks(SquirrelCoreServices& services) {
  services.validator = std::make_unique<const SquirrelValidator>(services);
  using V = SquirrelValidator;
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunctionExpr>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkLambda>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkConstructor>(validator)});
}

}  // namespace squirrel
