#include "golang/core/validation/GolangValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "golang/core/CoreServices.hpp"

namespace golang {

// No checks yet: registers the validator so the service exists, and nothing
// beyond it.
void register_validation_checks(GolangCoreServices& services) {
  using V = GolangValidator;
  services.validator = std::make_unique<const V>(services);
}

}  // namespace golang
