// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "p/core/CoreServices.hpp"
#include "p/core/validation/PValidator.hpp"
#include "p/generated/P/Module.hpp"

namespace p {

void install_core_module(PCoreServices& services) {
  p::generated::P::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace p
