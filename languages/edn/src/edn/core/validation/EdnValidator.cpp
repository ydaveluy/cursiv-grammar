#include "edn/core/validation/EdnValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "edn/core/CoreServices.hpp"

namespace edn {

void register_validation_checks(EdnCoreServices& services) {
  services.validator = std::make_unique<const EdnValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace edn
