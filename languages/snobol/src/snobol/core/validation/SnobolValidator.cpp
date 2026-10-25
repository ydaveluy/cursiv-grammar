#include "snobol/core/validation/SnobolValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "snobol/core/CoreServices.hpp"

namespace snobol {

// No checks yet: nothing here needs validation beyond parsing.
void register_validation_checks(SnobolCoreServices& services) {
  using V = SnobolValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace snobol
