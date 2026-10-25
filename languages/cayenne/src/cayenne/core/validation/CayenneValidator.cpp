#include "cayenne/core/validation/CayenneValidator.hpp"

#include <memory>

#include "cayenne/core/CoreServices.hpp"

namespace cayenne {

void register_validation_checks(CayenneCoreServices& services) {
  services.validator = std::make_unique<const CayenneValidator>(services);
}

}  // namespace cayenne
