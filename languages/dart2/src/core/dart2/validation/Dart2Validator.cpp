#include "dart2/validation/Dart2Validator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "dart2/CoreServices.hpp"

namespace dart2 {

void register_validation_checks(Dart2CoreServices& services) {
  using V = Dart2Validator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace dart2
