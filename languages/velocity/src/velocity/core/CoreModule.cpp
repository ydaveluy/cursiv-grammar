// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "velocity/core/CoreServices.hpp"
#include "velocity/core/validation/VelocityValidator.hpp"
#include "velocity/generated/Velocity/Module.hpp"

namespace velocity {

void install_core_module(VelocityCoreServices& services) {
  velocity::generated::Velocity::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace velocity
