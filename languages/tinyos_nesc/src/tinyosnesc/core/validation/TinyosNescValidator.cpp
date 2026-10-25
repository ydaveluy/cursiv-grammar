#include "tinyosnesc/core/validation/TinyosNescValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tinyosnesc/core/CoreServices.hpp"

namespace tinyosnesc {

void register_validation_checks(TinyosNescCoreServices& services) {
  services.validator =
      std::make_unique<const TinyosNescValidator>(services);
}

}  // namespace tinyosnesc
