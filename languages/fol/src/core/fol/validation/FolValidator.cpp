#include "fol/validation/FolValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "fol/CoreServices.hpp"

namespace fol {

void register_validation_checks(FolCoreServices& services) {
  services.validator = std::make_unique<const FolValidator>(services);
}

}  // namespace fol
