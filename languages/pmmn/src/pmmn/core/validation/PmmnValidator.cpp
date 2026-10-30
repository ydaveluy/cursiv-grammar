#include "pmmn/core/validation/PmmnValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pmmn/core/CoreServices.hpp"

namespace pmmn {

void PmmnValidator::checkInc(const ast::Inc& node, cursiv::ValidationAcceptor& accept) const {
  if (node.counter > 2000000000ULL)
    accept.error(node, "Counter identifier must not exceed 2,000,000,000.", "pmmn.counter-range").property<&ast::Inc::counter>();
}
void PmmnValidator::checkTest(const ast::Test& node, cursiv::ValidationAcceptor& accept) const {
  if (node.counter > 2000000000ULL)
    accept.error(node, "Counter identifier must not exceed 2,000,000,000.", "pmmn.counter-range").property<&ast::Test::counter>();
}

void register_validation_checks(PmmnCoreServices& services) {
  services.validator = std::make_unique<const PmmnValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&PmmnValidator::checkInc>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&PmmnValidator::checkTest>(*services.validator)});
}

}  // namespace pmmn
