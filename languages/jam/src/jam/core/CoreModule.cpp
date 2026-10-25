// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "jam/core/CoreServices.hpp"
#include "jam/core/validation/JamValidator.hpp"
#include "jam/generated/Jam/Module.hpp"

namespace jam {

void install_core_module(JamCoreServices& services) {
  jam::generated::Jam::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace jam
