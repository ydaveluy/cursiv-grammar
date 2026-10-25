#include "plantuml/core/validation/PlantUMLValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "plantuml/core/CoreServices.hpp"

namespace plantuml {

// No check is registered yet, but the slot and the container both exist so a
// future one has somewhere to land.
void register_validation_checks(PlantUMLCoreServices& services) {
  using V = PlantUMLValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace plantuml
