#include "powerquery/core/validation/PowerqueryValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "powerquery/core/CoreServices.hpp"

namespace powerquery {

namespace {
template <class Params>
void checkOrder(const Params& params, cursiv::ValidationAcceptor& accept) {
  bool seenOptional = false;
  for (const auto* param : params) {
    if (!param) continue;
    if (param->optional)
      seenOptional = true;
    else if (seenOptional)
      accept.error(*param,
                   "A required parameter cannot follow an optional parameter.");
  }
}
}  // namespace
void PowerqueryValidator::checkParamOrder(
    const ast::FunctionExpr& node, cursiv::ValidationAcceptor& accept) const {
  checkOrder(node.params, accept);
}
void PowerqueryValidator::checkTypeParamOrder(
    const ast::FunctionTypeNode& node,
    cursiv::ValidationAcceptor& accept) const {
  checkOrder(node.params, accept);
}
void register_validation_checks(PowerqueryCoreServices& services) {
  services.validator = std::make_unique<const PowerqueryValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &PowerqueryValidator::checkParamOrder>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &PowerqueryValidator::checkTypeParamOrder>(*services.validator)});
}

}  // namespace powerquery
