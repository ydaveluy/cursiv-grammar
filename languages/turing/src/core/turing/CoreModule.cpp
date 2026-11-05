// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "turing/CoreServices.hpp"
#include "turing/validation/TuringValidator.hpp"
#include "turing/generated/Turing/Module.hpp"

namespace turing {

void install_core_module(TuringCoreServices& services) {
  turing::generated::Turing::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace turing
