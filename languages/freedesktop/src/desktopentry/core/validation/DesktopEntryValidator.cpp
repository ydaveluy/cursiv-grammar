#include "desktopentry/core/validation/DesktopEntryValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "desktopentry/core/CoreServices.hpp"

namespace desktopentry {

void register_validation_checks(DesktopEntryCoreServices& services) {
  services.validator =
      std::make_unique<const DesktopEntryValidator>(services);
}

}  // namespace desktopentry
