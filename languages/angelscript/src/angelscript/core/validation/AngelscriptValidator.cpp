#include "angelscript/core/validation/AngelscriptValidator.hpp"

#include <memory>

#include "angelscript/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace angelscript {

// Host-registered types, overload resolution and engine configuration are
// outside this syntax port. No semantic checks are registered here.
void register_validation_checks(AngelscriptCoreServices& services) {
  services.validator =
      std::make_unique<const AngelscriptValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace angelscript
