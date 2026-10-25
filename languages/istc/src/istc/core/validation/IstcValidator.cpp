#include "istc/core/validation/IstcValidator.hpp"

#include <memory>

#include "istc/core/CoreServices.hpp"

namespace istc {

void register_validation_checks(IstcCoreServices& services) {
  services.validator = std::make_unique<const IstcValidator>(services);
}

}  // namespace istc
