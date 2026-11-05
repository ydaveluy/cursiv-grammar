#include "ltl/validation/LtlValidator.hpp"

#include <memory>

#include "ltl/CoreServices.hpp"

namespace ltl {

void register_validation_checks(LtlCoreServices& services) {
  services.validator = std::make_unique<const LtlValidator>(services);
}

}  // namespace ltl
