// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "moo/core/CoreServices.hpp"
#include "moo/core/validation/MooValidator.hpp"
#include "moo/generated/Moo/Module.hpp"

namespace moo {

void install_core_module(MooCoreServices& services) {
  moo::generated::Moo::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace moo
