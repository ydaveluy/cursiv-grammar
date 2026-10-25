#include "ltl/core/validation/LtlValidator.hpp"

#include <memory>

#include "ltl/core/CoreServices.hpp"

namespace ltl {

void register_validation_checks(LtlCoreServices& services) {
  services.validator = std::make_unique<const LtlValidator>(services);
}

}  // namespace ltl
