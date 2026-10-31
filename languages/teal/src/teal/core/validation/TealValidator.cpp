#include "teal/core/validation/TealValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "teal/core/CoreServices.hpp"

namespace teal {

void TealValidator::checkNewType(const ast::NewType& node,
                                 cursiv::ValidationAcceptor& accept) const {
  if (node.legacy)
    accept.error(node, "Use a named record/enum declaration or a type alias.", "teal.legacy-type");
}
void TealValidator::checkRecord(const ast::RecordNewType& node,
                              cursiv::ValidationAcceptor& accept) const {
  if (!node.isInterface) return;
  for (const auto* nested : node.nestedTypes) {
    if (!nested) continue;
    if (const auto* record = cursiv::ast_cast<ast::RecordNewType>(nested->value);
        record && !record->isInterface)
      accept.error(*nested, "Interfaces cannot contain record definitions.",
                   "teal.interface-record");
  }
}
void register_validation_checks(TealCoreServices& services) {
  services.validator = std::make_unique<const TealValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&TealValidator::checkNewType>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&TealValidator::checkRecord>(*services.validator)});
}

}  // namespace teal
