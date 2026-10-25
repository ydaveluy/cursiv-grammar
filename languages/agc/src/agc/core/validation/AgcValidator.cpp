#include "agc/core/validation/AgcValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "agc/core/CoreServices.hpp"

namespace agc {

void register_validation_checks(AgcCoreServices& services) {
  services.validator = std::make_unique<const AgcValidator>(services);
}

}  // namespace agc
