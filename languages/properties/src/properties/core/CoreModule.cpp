// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "properties/core/CoreServices.hpp"
#include "properties/core/validation/PropertiesValidator.hpp"
#include "properties/generated/Properties/Module.hpp"

namespace properties {

void install_core_module(PropertiesCoreServices& services) {
  properties::generated::Properties::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace properties
