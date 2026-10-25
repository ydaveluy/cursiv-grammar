#include "tinymud/core/validation/TinymudValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tinymud/core/CoreServices.hpp"

namespace tinymud {

void register_validation_checks(TinymudCoreServices& services) {
  services.validator = std::make_unique<const TinymudValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace tinymud
