// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "flowmatic/core/CoreServices.hpp"
#include "flowmatic/core/validation/FlowmaticValidator.hpp"
#include "flowmatic/generated/Flowmatic/Module.hpp"

namespace flowmatic {

void install_core_module(FlowmaticCoreServices& services) {
  flowmatic::generated::Flowmatic::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace flowmatic
