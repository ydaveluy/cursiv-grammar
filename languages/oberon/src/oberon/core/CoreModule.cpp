// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "oberon/core/CoreServices.hpp"
#include "oberon/core/validation/OberonValidator.hpp"
#include "oberon/generated/Oberon/Module.hpp"

namespace oberon {

void install_core_module(OberonCoreServices& services) {
  oberon::generated::Oberon::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace oberon
