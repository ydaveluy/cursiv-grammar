// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "prolog/CoreServices.hpp"
#include "prolog/validation/PrologValidator.hpp"
#include "prolog/generated/Prolog/Module.hpp"

namespace prolog {

void install_core_module(PrologCoreServices& services) {
  prolog::generated::Prolog::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace prolog
