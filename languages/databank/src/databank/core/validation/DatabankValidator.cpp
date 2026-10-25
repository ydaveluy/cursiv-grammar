#include "databank/core/validation/DatabankValidator.hpp"

#include <memory>

#include "databank/core/CoreServices.hpp"

namespace databank {

void register_validation_checks(DatabankCoreServices& services) {
  services.validator = std::make_unique<const DatabankValidator>(services);
}

}  // namespace databank
