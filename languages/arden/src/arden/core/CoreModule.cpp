// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "arden/core/CoreServices.hpp"
#include "arden/core/validation/ArdenValidator.hpp"
#include "arden/generated/Arden/Module.hpp"

namespace arden {

void install_core_module(ArdenCoreServices& services) {
  arden::generated::Arden::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace arden
