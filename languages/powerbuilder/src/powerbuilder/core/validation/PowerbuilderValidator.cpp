#include "powerbuilder/core/validation/PowerbuilderValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "powerbuilder/core/CoreServices.hpp"

namespace powerbuilder {

void register_validation_checks(PowerbuilderCoreServices& services) {
  services.validator = std::make_unique<const PowerbuilderValidator>(services);
}

}  // namespace powerbuilder
