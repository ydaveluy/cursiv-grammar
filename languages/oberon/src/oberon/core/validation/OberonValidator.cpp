#include "oberon/core/validation/OberonValidator.hpp"

#include <memory>

#include "oberon/core/CoreServices.hpp"

namespace oberon {

void register_validation_checks(OberonCoreServices& services) {
  services.validator = std::make_unique<const OberonValidator>(services);
}

}  // namespace oberon
