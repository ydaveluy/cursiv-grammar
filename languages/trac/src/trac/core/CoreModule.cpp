// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "trac/core/CoreServices.hpp"
#include "trac/core/validation/TracValidator.hpp"
#include "trac/generated/Trac/Module.hpp"

namespace trac {

void install_core_module(TracCoreServices& services) {
  trac::generated::Trac::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace trac
