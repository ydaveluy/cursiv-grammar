#include "aql/core/validation/AqlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "aql/core/CoreServices.hpp"

namespace aql {

void register_validation_checks(AqlCoreServices& services) {
  using V = AqlValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace aql
