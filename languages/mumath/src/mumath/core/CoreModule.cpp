// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "mumath/core/CoreServices.hpp"
#include "mumath/core/validation/MumathValidator.hpp"
#include "mumath/generated/Mumath/Module.hpp"

namespace mumath {

void install_core_module(MumathCoreServices& services) {
  mumath::generated::Mumath::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace mumath
