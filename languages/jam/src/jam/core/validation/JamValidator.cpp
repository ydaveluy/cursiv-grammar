#include "jam/core/validation/JamValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "jam/core/CoreServices.hpp"

namespace jam {

void JamValidator::checkMeter(const ast::Meter& meter,
                              cursiv::ValidationAcceptor& accept) const {
  if (meter.numerator < 1 || meter.numerator > 256 ||
      meter.denominator < 1 || meter.denominator > 256)
    accept.error(meter, "Meter components must be between 1 and 256.", "jam.meter");
}

void register_validation_checks(JamCoreServices& services) {
  services.validator = std::make_unique<const JamValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&JamValidator::checkMeter>(*services.validator)});
}

}  // namespace jam
