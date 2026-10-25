#include "tnsnames/core/validation/TnsnamesValidator.hpp"

#include <memory>

#include "tnsnames/core/CoreServices.hpp"

namespace tnsnames {

void register_validation_checks(TnsnamesCoreServices& services) {
  services.validator = std::make_unique<const TnsnamesValidator>(services);
}

}  // namespace tnsnames
