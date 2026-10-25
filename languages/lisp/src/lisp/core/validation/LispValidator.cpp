#include "lisp/core/validation/LispValidator.hpp"

#include <memory>

#include "lisp/core/CoreServices.hpp"

namespace lisp {

void register_validation_checks(LispCoreServices& services) {
  services.validator = std::make_unique<const LispValidator>(services);
}

}  // namespace lisp
