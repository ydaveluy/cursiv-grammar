#include "algol60/core/validation/Algol60Validator.hpp"

#include <memory>

#include "algol60/core/CoreServices.hpp"

namespace algol60 {

void register_validation_checks(Algol60CoreServices& services) {
  services.validator = std::make_unique<const Algol60Validator>(services);
}

}  // namespace algol60
