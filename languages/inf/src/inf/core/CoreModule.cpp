// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "inf/core/CoreServices.hpp"
#include "inf/core/validation/InfValidator.hpp"
#include "inf/generated/Inf/Module.hpp"

namespace inf {

void install_core_module(InfCoreServices& services) {
  inf::generated::Inf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace inf
