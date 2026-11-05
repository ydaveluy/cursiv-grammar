#include "databank/validation/DatabankValidator.hpp"

#include <memory>

#include "databank/CoreServices.hpp"

namespace databank {

void register_validation_checks(DatabankCoreServices& services) {
  services.validator = std::make_unique<const DatabankValidator>(services);
}

}  // namespace databank
