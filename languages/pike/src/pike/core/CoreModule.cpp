// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pike/core/CoreServices.hpp"
#include "pike/core/validation/PikeValidator.hpp"
#include "pike/generated/Pike/Module.hpp"

namespace pike {

void install_core_module(PikeCoreServices& services) {
  pike::generated::Pike::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pike
