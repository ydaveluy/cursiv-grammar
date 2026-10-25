#include "creole/core/validation/CreoleValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "creole/core/CoreServices.hpp"

namespace creole {

// No check registered: nothing here.
void register_validation_checks(CreoleCoreServices& services) {
  using V = CreoleValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace creole
