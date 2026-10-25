// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "mumps/core/CoreServices.hpp"
#include "mumps/core/validation/MumpsValidator.hpp"
#include "mumps/generated/Mumps/Module.hpp"

namespace mumps {

void install_core_module(MumpsCoreServices& services) {
  mumps::generated::Mumps::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace mumps
