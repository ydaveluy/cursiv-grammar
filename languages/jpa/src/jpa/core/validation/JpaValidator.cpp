#include "jpa/core/validation/JpaValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "jpa/core/CoreServices.hpp"

namespace jpa {

void register_validation_checks(JpaCoreServices& services) {
  services.validator = std::make_unique<const JpaValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace jpa
