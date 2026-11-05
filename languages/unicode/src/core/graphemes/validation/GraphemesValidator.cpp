#include "graphemes/validation/GraphemesValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "graphemes/CoreServices.hpp"

namespace graphemes {

void register_validation_checks(GraphemesCoreServices& services) {
  using V = GraphemesValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace graphemes
