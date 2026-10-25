#include "fol/core/validation/FolValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "fol/core/CoreServices.hpp"

namespace fol {

void register_validation_checks(FolCoreServices& services) {
  services.validator = std::make_unique<const FolValidator>(services);
}

}  // namespace fol
