#include "scala/core/validation/ScalaValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "scala/core/CoreServices.hpp"

namespace scala {

void register_validation_checks(ScalaCoreServices& services) {
  services.validator = std::make_unique<const ScalaValidator>(services);
}

}  // namespace scala
