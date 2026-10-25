#include "guitartab/core/validation/GuitartabValidator.hpp"

#include <memory>

#include "guitartab/core/CoreServices.hpp"

namespace guitartab {

// No checks to register yet; the container still needs the validator built.
void register_validation_checks(GuitartabCoreServices& services) {
  services.validator = std::make_unique<const GuitartabValidator>(services);
}

}  // namespace guitartab
