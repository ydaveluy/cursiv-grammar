#include "agc/validation/AgcValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "agc/CoreServices.hpp"

namespace agc {

void register_validation_checks(AgcCoreServices& services) {
  services.validator = std::make_unique<const AgcValidator>(services);
}

}  // namespace agc
