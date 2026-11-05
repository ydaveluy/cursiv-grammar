#include "fusiontables/validation/FusionTablesValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "fusiontables/CoreServices.hpp"

namespace fusiontables {

// This teaching grammar has no native table/type/name/statement validation.
void register_validation_checks(FusionTablesCoreServices& services) {
  services.validator =
      std::make_unique<const FusionTablesValidator>(services);
}

}  // namespace fusiontables
