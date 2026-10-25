#include "fortran/core/validation/FortranValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "fortran/core/CoreServices.hpp"

namespace fortran {

void register_validation_checks(FortranCoreServices& services) {
  services.validator = std::make_unique<const FortranValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace fortran
