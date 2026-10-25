#include "caql/core/validation/CaqlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "caql/core/CoreServices.hpp"

namespace caql {

void register_validation_checks(CaqlCoreServices& services) {
  using V = CaqlValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace caql
