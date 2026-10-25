#include "angelscript/core/validation/AngelscriptValidator.hpp"

#include <memory>

#include "angelscript/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace angelscript {

// No check yet: the grammar accepts everything the language does, so there
// is nothing here for a validator to narrow.
void register_validation_checks(AngelscriptCoreServices& services) {
  services.validator =
      std::make_unique<const AngelscriptValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace angelscript
