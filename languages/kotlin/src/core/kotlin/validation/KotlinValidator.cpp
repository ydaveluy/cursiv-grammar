#include "kotlin/validation/KotlinValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "kotlin/CoreServices.hpp"

namespace kotlin {

void register_validation_checks(KotlinCoreServices& services) {
  services.validator = std::make_unique<const KotlinValidator>(services);
}

}  // namespace kotlin
