#include "propcalc/core/validation/PropcalcValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "propcalc/core/CoreServices.hpp"

namespace propcalc {

void PropcalcValidator::checkVariable(const ast::VariableRef& node,
                                      cursiv::ValidationAcceptor& accept) const {
  if (node.name.empty())
    accept.error(node, "Missing operand.", "propcalc.operand");
  for (std::size_t i = 0; i < node.extraNames.size(); ++i)
    accept.error(node, "Missing connective between variables.", "propcalc.operator")
        .property<&ast::VariableRef::extraNames>(i);
}

void register_validation_checks(PropcalcCoreServices& services) {
  services.validator = std::make_unique<const PropcalcValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&PropcalcValidator::checkVariable>(*services.validator)});
}

}  // namespace propcalc
