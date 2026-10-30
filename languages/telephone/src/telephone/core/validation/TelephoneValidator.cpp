#include "telephone/core/validation/TelephoneValidator.hpp"

#include <memory>

#include "telephone/core/CoreServices.hpp"

namespace telephone {

void register_validation_checks(TelephoneCoreServices& services) {
  services.validator = std::make_unique<const TelephoneValidator>(services);
}

}  // namespace telephone
