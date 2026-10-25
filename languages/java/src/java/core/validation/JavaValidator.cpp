#include "java/core/validation/JavaValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "java/core/CoreServices.hpp"

namespace java {

// The grammar accepts `...` on any component; only the last may vary.
void JavaValidator::checkRecordComponentVarargsIsLast(
    const ast::RecordDecl& record, cursiv::ValidationAcceptor& accept) const {
  const auto count = record.components.size();
  for (std::size_t i = 0; i + 1 < count; ++i) {
    const ast::RecordComponent* const component = record.components[i];
    if (component != nullptr && component->varargs)
      accept.error(*component, "Only the last record component may be varargs.");
  }
}

void register_validation_checks(JavaCoreServices& services) {
  using V = JavaValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkRecordComponentVarargsIsLast>(validator)});
}

}  // namespace java
