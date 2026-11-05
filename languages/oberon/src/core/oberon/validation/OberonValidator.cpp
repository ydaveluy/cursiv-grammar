#include "oberon/validation/OberonValidator.hpp"

#include <memory>

#include "oberon/CoreServices.hpp"

namespace oberon {

void register_validation_checks(OberonCoreServices& services) {
  services.validator = std::make_unique<const OberonValidator>(services);
}

}  // namespace oberon
