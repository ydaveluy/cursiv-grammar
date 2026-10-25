#include "spass/core/validation/SpassValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "spass/core/CoreServices.hpp"

namespace spass {

void register_validation_checks(SpassCoreServices& services) {
  services.validator = std::make_unique<const SpassValidator>(services);
}

}  // namespace spass
