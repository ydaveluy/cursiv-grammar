#include "asn/core/validation/AsnValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "asn/core/CoreServices.hpp"

namespace asn {

void register_validation_checks(AsnCoreServices& services) {
  using V = AsnValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace asn
