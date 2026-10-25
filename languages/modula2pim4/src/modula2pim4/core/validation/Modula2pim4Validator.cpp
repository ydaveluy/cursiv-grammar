#include "modula2pim4/core/validation/Modula2pim4Validator.hpp"

#include <memory>

#include "modula2pim4/core/CoreServices.hpp"

namespace modula2pim4 {

void register_validation_checks(Modula2pim4CoreServices& services) {
  services.validator = std::make_unique<const Modula2pim4Validator>(services);
}

}  // namespace modula2pim4
