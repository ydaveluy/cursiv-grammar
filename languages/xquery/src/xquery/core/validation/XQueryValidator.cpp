#include "xquery/core/validation/XQueryValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "xquery/core/CoreServices.hpp"

namespace xquery {

void register_validation_checks(XQueryCoreServices& services) {
  using V = XQueryValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace xquery
