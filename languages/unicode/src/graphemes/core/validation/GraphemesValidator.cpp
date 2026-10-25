#include "graphemes/core/validation/GraphemesValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "graphemes/core/CoreServices.hpp"

namespace graphemes {

void register_validation_checks(GraphemesCoreServices& services) {
  using V = GraphemesValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace graphemes
