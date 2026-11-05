#include "haskell/validation/HaskellValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "haskell/CoreServices.hpp"

namespace haskell {

void register_validation_checks(HaskellCoreServices& services) {
  using V = HaskellValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace haskell
