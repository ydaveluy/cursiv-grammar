#include "squirrel/core/validation/SquirrelValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "squirrel/core/CoreServices.hpp"

namespace squirrel {

void register_validation_checks(SquirrelCoreServices& services) {
  services.validator = std::make_unique<const SquirrelValidator>(services);
}

}  // namespace squirrel
