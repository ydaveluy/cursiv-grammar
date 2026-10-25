#include "dcm/core/validation/DcmValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "dcm/core/CoreServices.hpp"

namespace dcm {

void register_validation_checks(DcmCoreServices& services) {
  using V = DcmValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace dcm
