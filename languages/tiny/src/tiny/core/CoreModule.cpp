// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tiny/core/CoreServices.hpp"
#include "tiny/core/validation/TinyValidator.hpp"
#include "tiny/generated/Tiny/Module.hpp"

namespace tiny {

void install_core_module(TinyCoreServices& services) {
  tiny::generated::Tiny::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tiny
