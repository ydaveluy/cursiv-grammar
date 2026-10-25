#include "modelica/core/validation/ModelicaValidator.hpp"

#include <memory>

#include "modelica/core/CoreServices.hpp"

namespace modelica {

// Nothing registered yet: the port is syntax-only.
void register_validation_checks(ModelicaCoreServices& services) {
  services.validator = std::make_unique<const ModelicaValidator>(services);
}

}  // namespace modelica
