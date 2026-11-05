#include "caql/validation/CaqlValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "caql/CoreServices.hpp"

namespace caql {

void register_validation_checks(CaqlCoreServices& services) {
  using V = CaqlValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace caql
