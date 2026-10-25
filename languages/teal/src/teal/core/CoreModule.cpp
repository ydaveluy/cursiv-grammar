// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "teal/core/CoreServices.hpp"
#include "teal/core/validation/TealValidator.hpp"
#include "teal/generated/Teal/Module.hpp"

namespace teal {

void install_core_module(TealCoreServices& services) {
  teal::generated::Teal::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace teal
