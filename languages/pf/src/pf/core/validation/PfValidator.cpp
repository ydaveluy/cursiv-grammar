#include "pf/core/validation/PfValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pf/core/CoreServices.hpp"

namespace pf {

void register_validation_checks(PfCoreServices& services) {
  services.validator = std::make_unique<const PfValidator>(services);
}

}  // namespace pf
