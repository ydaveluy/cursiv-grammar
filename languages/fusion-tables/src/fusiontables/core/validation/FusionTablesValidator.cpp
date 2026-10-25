#include "fusiontables/core/validation/FusionTablesValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "fusiontables/core/CoreServices.hpp"

namespace fusiontables {

// No checks yet: the grammar carries the whole language, nothing here needs
// validating beyond parsing.
void register_validation_checks(FusionTablesCoreServices& services) {
  services.validator =
      std::make_unique<const FusionTablesValidator>(services);
}

}  // namespace fusiontables
