// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "argus/core/CoreServices.hpp"
#include "argus/core/validation/ArgusValidator.hpp"
#include "argus/generated/Argus/Module.hpp"

namespace argus {

void install_core_module(ArgusCoreServices& services) {
  argus::generated::Argus::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace argus
