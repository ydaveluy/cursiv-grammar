#include "refal/core/validation/RefalValidator.hpp"

#include <memory>

#include "refal/core/CoreServices.hpp"

namespace refal {

void register_validation_checks(RefalCoreServices& services) {
  services.validator = std::make_unique<const RefalValidator>(services);
}

}  // namespace refal
