#include "janus/core/validation/JanusValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "janus/core/CoreServices.hpp"

namespace janus {

void register_validation_checks(JanusCoreServices& services) {
  using V = JanusValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace janus
