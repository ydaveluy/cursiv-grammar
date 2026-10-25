#include "joss/core/validation/JossValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "joss/core/CoreServices.hpp"

namespace joss {

void register_validation_checks(JossCoreServices& services) {
  services.validator = std::make_unique<const JossValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace joss
