#include "scotty/core/validation/ScottyValidator.hpp"

#include <memory>

#include "scotty/core/CoreServices.hpp"

namespace scotty {

void register_validation_checks(ScottyCoreServices& services) {
  services.validator = std::make_unique<const ScottyValidator>(services);
}

}  // namespace scotty
