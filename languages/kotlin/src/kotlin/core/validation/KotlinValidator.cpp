#include "kotlin/core/validation/KotlinValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "kotlin/core/CoreServices.hpp"

namespace kotlin {

void register_validation_checks(KotlinCoreServices& services) {
  services.validator = std::make_unique<const KotlinValidator>(services);
}

}  // namespace kotlin
