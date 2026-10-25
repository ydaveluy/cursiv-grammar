// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "icon/core/CoreServices.hpp"
#include "icon/core/validation/IconValidator.hpp"
#include "icon/generated/Icon/Module.hpp"

namespace icon {

void install_core_module(IconCoreServices& services) {
  icon::generated::Icon::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace icon
