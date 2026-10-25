// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "robotwars/core/CoreServices.hpp"
#include "robotwars/core/validation/RobotwarsValidator.hpp"
#include "robotwars/generated/Robotwars/Module.hpp"

namespace robotwars {

void install_core_module(RobotwarsCoreServices& services) {
  robotwars::generated::Robotwars::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace robotwars
