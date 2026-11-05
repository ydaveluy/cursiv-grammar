#include "vb6/validation/Vb6Validator.hpp"

#include <memory>

#include "vb6/CoreServices.hpp"

namespace vb6 {

void register_validation_checks(Vb6CoreServices& services) {
  services.validator = std::make_unique<const Vb6Validator>(services);
}

}  // namespace vb6
