#include "python/core/validation/PythonValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "python/core/CoreServices.hpp"

namespace python {

void register_validation_checks(PythonCoreServices& services) {
  services.validator = std::make_unique<const PythonValidator>(services);
}

}  // namespace python
