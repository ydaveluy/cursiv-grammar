#include "powerbuilderdw/core/validation/PowerbuilderdwValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "powerbuilderdw/core/CoreServices.hpp"

namespace powerbuilderdw {

void register_validation_checks(PowerbuilderdwCoreServices& services) {
  using V = PowerbuilderdwValidator;
  services.validator = std::make_unique<const V>(services);
}

}  // namespace powerbuilderdw
