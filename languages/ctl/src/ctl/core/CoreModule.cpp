// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "ctl/core/CoreServices.hpp"
#include "ctl/core/validation/CtlValidator.hpp"
#include "ctl/generated/Ctl/Module.hpp"

namespace ctl {

void install_core_module(CtlCoreServices& services) {
  ctl::generated::Ctl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace ctl
