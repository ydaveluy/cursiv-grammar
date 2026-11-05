#include "peoplecode/validation/PeoplecodeValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "peoplecode/CoreServices.hpp"

namespace peoplecode {

void register_validation_checks(PeoplecodeCoreServices& services) {
  services.validator =
      std::make_unique<const PeoplecodeValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace peoplecode
