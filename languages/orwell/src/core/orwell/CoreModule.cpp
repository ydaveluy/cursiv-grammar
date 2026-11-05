// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "orwell/CoreServices.hpp"
#include "orwell/validation/OrwellValidator.hpp"
#include "orwell/generated/Orwell/Module.hpp"

namespace orwell {

void install_core_module(OrwellCoreServices& services) {
  orwell::generated::Orwell::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace orwell
