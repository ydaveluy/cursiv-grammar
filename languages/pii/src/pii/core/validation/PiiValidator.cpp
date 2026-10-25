#include "pii/core/validation/PiiValidator.hpp"

#include <memory>

#include "pii/core/CoreServices.hpp"

namespace pii {

void register_validation_checks(PiiCoreServices& services) {
  services.validator = std::make_unique<const PiiValidator>(services);
}

}  // namespace pii
