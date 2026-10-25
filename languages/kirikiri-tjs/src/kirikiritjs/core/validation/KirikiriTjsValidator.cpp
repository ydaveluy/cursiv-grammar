#include "kirikiritjs/core/validation/KirikiriTjsValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "kirikiritjs/core/CoreServices.hpp"

namespace kirikiritjs {

void register_validation_checks(KirikiriTjsCoreServices& services) {
  services.validator = std::make_unique<const KirikiriTjsValidator>(services);
}

}  // namespace kirikiritjs
