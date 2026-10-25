#include "trapc/core/validation/TrapcValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "trapc/core/CoreServices.hpp"

namespace trapc {

void register_validation_checks(TrapcCoreServices& services) {
  services.validator = std::make_unique<const TrapcValidator>(services);
}

}  // namespace trapc
