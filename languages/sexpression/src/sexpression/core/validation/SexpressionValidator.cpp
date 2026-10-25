#include "sexpression/core/validation/SexpressionValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sexpression/core/CoreServices.hpp"

namespace sexpression {

void register_validation_checks(SexpressionCoreServices& services) {
  using V = SexpressionValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace sexpression
