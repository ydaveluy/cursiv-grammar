#include "aspectj/core/validation/AspectjValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "aspectj/core/CoreServices.hpp"

namespace aspectj {

// The grammar accepts `...` on any component; only the last may vary.
void AspectjValidator::checkRecordComponentVarargsIsLast(
    const ast::RecordDecl& record, cursiv::ValidationAcceptor& accept) const {
  const auto count = record.components.size();
  for (std::size_t i = 0; i + 1 < count; ++i) {
    const ast::RecordComponent* const component = record.components[i];
    if (component != nullptr && component->varargs)
      accept.error(*component, "Only the last record component may be varargs.");
  }
}

void register_validation_checks(AspectjCoreServices& services) {
  using V = AspectjValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkRecordComponentVarargsIsLast>(validator)});
}

}  // namespace aspectj
