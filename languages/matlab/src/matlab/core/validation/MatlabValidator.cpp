#include "matlab/core/validation/MatlabValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "matlab/core/CoreServices.hpp"

namespace matlab {

void register_validation_checks(MatlabCoreServices& services) {
  services.validator = std::make_unique<const MatlabValidator>(services);
}

}  // namespace matlab
