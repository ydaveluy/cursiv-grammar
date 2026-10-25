// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "creole/core/CoreServices.hpp"
#include "creole/core/validation/CreoleValidator.hpp"
#include "creole/generated/Creole/Module.hpp"

namespace creole {

void install_core_module(CreoleCoreServices& services) {
  creole::generated::Creole::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace creole
