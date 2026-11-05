#include "idl/validation/IdlValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "idl/CoreServices.hpp"

namespace idl {

void register_validation_checks(IdlCoreServices& services) {
  services.validator = std::make_unique<const IdlValidator>(services);
}

}  // namespace idl
