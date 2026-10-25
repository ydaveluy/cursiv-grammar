// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "rcs/core/CoreServices.hpp"
#include "rcs/core/validation/RcsValidator.hpp"
#include "rcs/generated/Rcs/Module.hpp"

namespace rcs {

void install_core_module(RcsCoreServices& services) {
  rcs::generated::Rcs::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace rcs
