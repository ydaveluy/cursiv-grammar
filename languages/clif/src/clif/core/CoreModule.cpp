// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "clif/core/CoreServices.hpp"
#include "clif/core/validation/ClifValidator.hpp"
#include "clif/generated/Clif/Module.hpp"

namespace clif {

void install_core_module(ClifCoreServices& services) {
  clif::generated::Clif::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace clif
