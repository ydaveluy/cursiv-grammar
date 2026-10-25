// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "informix/core/CoreServices.hpp"
#include "informix/core/validation/InformixValidator.hpp"
#include "informix/generated/Informix/Module.hpp"

namespace informix {

void install_core_module(InformixCoreServices& services) {
  informix::generated::Informix::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace informix
