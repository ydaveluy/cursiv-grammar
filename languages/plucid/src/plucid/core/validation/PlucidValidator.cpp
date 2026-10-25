#include "plucid/core/validation/PlucidValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "plucid/core/CoreServices.hpp"

namespace plucid {

void register_validation_checks(PlucidCoreServices& services) {
  services.validator = std::make_unique<const PlucidValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace plucid
