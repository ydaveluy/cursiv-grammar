#include "moo/validation/MooValidator.hpp"

#include <memory>

#include "moo/CoreServices.hpp"

namespace moo {

void register_validation_checks(MooCoreServices& services) {
  services.validator = std::make_unique<const MooValidator>(services);
}

}  // namespace moo
