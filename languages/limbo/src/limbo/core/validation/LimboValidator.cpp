#include "limbo/core/validation/LimboValidator.hpp"

#include <memory>

#include "limbo/core/CoreServices.hpp"

namespace limbo {

void register_validation_checks(LimboCoreServices& services) {
  services.validator = std::make_unique<const LimboValidator>(services);
}

}  // namespace limbo
