// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "powerbuilder/core/CoreServices.hpp"
#include "powerbuilder/core/validation/PowerbuilderValidator.hpp"
#include "powerbuilder/generated/Powerbuilder/Module.hpp"

namespace powerbuilder {

void install_core_module(PowerbuilderCoreServices& services) {
  powerbuilder::generated::Powerbuilder::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace powerbuilder
