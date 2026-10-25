#include "alpaca/core/validation/AlpacaValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "alpaca/core/CoreServices.hpp"

namespace alpaca {

void register_validation_checks(AlpacaCoreServices& services) {
  services.validator = std::make_unique<const AlpacaValidator>(services);
}

}  // namespace alpaca
