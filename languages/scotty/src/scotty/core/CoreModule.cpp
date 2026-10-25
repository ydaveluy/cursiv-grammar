// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "scotty/core/CoreServices.hpp"
#include "scotty/core/validation/ScottyValidator.hpp"
#include "scotty/generated/Scotty/Module.hpp"

namespace scotty {

void install_core_module(ScottyCoreServices& services) {
  scotty::generated::Scotty::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace scotty
