#include "trapc/validation/TrapcValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "trapc/CoreServices.hpp"

namespace trapc {

void register_validation_checks(TrapcCoreServices& services) {
  services.validator = std::make_unique<const TrapcValidator>(services);
}

}  // namespace trapc
