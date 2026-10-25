// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "janus/core/CoreServices.hpp"
#include "janus/core/validation/JanusValidator.hpp"
#include "janus/generated/Janus/Module.hpp"

namespace janus {

void install_core_module(JanusCoreServices& services) {
  janus::generated::Janus::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace janus
