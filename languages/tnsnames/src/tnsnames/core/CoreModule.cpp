// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tnsnames/core/CoreServices.hpp"
#include "tnsnames/core/validation/TnsnamesValidator.hpp"
#include "tnsnames/generated/Tnsnames/Module.hpp"

namespace tnsnames {

void install_core_module(TnsnamesCoreServices& services) {
  tnsnames::generated::Tnsnames::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tnsnames
