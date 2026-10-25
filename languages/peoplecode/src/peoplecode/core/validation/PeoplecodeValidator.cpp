#include "peoplecode/core/validation/PeoplecodeValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "peoplecode/core/CoreServices.hpp"

namespace peoplecode {

void register_validation_checks(PeoplecodeCoreServices& services) {
  services.validator =
      std::make_unique<const PeoplecodeValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace peoplecode
