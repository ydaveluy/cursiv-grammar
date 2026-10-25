#include "idl/core/validation/IdlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "idl/core/CoreServices.hpp"

namespace idl {

void register_validation_checks(IdlCoreServices& services) {
  services.validator = std::make_unique<const IdlValidator>(services);
}

}  // namespace idl
