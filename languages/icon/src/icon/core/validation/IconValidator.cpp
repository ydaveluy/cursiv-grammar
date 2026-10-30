#include "icon/core/validation/IconValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "icon/core/CoreServices.hpp"

namespace icon {

void IconValidator::checkProcedure(const ast::Procedure& procedure,
                                    cursiv::ValidationAcceptor& accept) const {
  for (std::size_t i = 0; i + 1 < procedure.params.size(); ++i)
    if (procedure.params[i]->vararg)
      accept.error(*procedure.params[i], "The variable argument parameter must appear last.",
                   "icon.vararg");
}
void IconValidator::checkInvocable(const ast::InvocableItem& item,
                                    cursiv::ValidationAcceptor& accept) const {
  if (!item.arity.empty() && !item.name.starts_with('"'))
    accept.error(item, "An invocable arity applies to a quoted operator name.",
                 "icon.invocable");
}
void IconValidator::checkSuspend(const ast::SuspendExpr& expression,
                                 cursiv::ValidationAcceptor& accept) const {
  if (expression.doClause && !expression.value)
    accept.error(expression, "A suspend do-clause requires a value expression.",
                 "icon.suspend");
}

void register_validation_checks(IconCoreServices& services) {
  services.validator = std::make_unique<const IconValidator>(services);
  using V = IconValidator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkProcedure>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkInvocable>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkSuspend>(*services.validator)});
}

}  // namespace icon
