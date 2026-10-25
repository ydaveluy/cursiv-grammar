#include "telephone/core/validation/TelephoneValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "telephone/core/CoreServices.hpp"

namespace telephone {

// NANP area codes and exchange codes are both "NXX": N (the first digit)
// is 2-9, never 0 or 1.
void TelephoneValidator::checkCentralOfficeCodes(
    const ast::Nanp& nanp, cursiv::ValidationAcceptor& accept) const {
  if (!nanp.areaCode.empty() &&
      (nanp.areaCode.front() == '0' || nanp.areaCode.front() == '1'))
    accept.warning(nanp, "NANP area codes do not start with '0' or '1'.")
        .property<&ast::Variation::areaCode>();
  if (!nanp.exchange.empty() &&
      (nanp.exchange.front() == '0' || nanp.exchange.front() == '1'))
    accept.warning(nanp, "NANP exchange codes do not start with '0' or '1'.")
        .property<&ast::Variation::exchange>();
}

void register_validation_checks(TelephoneCoreServices& services) {
  using V = TelephoneValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkCentralOfficeCodes>(validator)});
}

}  // namespace telephone
