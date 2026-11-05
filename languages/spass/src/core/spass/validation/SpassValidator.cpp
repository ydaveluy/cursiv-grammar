#include "spass/validation/SpassValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "spass/CoreServices.hpp"

namespace spass {

void register_validation_checks(SpassCoreServices& services) {
  services.validator = std::make_unique<const SpassValidator>(services);
}

}  // namespace spass
