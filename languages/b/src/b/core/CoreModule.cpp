// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "b/core/CoreServices.hpp"
#include "b/core/validation/BValidator.hpp"
#include "b/generated/B/Module.hpp"

namespace b {

void install_core_module(BCoreServices& services) {
  b::generated::B::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace b
