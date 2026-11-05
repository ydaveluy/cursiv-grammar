#include "matlab/validation/MatlabValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "matlab/CoreServices.hpp"

namespace matlab {

void register_validation_checks(MatlabCoreServices& services) {
  services.validator = std::make_unique<const MatlabValidator>(services);
}

}  // namespace matlab
