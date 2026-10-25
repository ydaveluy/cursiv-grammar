#include "smalltalk/core/validation/SmalltalkValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "smalltalk/core/CoreServices.hpp"

namespace smalltalk {

void register_validation_checks(SmalltalkCoreServices& services) {
  services.validator = std::make_unique<const SmalltalkValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace smalltalk
