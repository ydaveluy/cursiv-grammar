#include "vb6/core/validation/Vb6Validator.hpp"

#include <memory>

#include "vb6/core/CoreServices.hpp"

namespace vb6 {

void register_validation_checks(Vb6CoreServices& services) {
  services.validator = std::make_unique<const Vb6Validator>(services);
}

}  // namespace vb6
