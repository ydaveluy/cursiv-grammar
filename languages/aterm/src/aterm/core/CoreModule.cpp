// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "aterm/core/CoreServices.hpp"
#include "aterm/core/validation/AtermValidator.hpp"
#include "aterm/generated/Aterm/Module.hpp"

namespace aterm {

void install_core_module(AtermCoreServices& services) {
  aterm::generated::Aterm::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace aterm
