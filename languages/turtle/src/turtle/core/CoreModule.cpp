// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "turtle/core/CoreServices.hpp"
#include "turtle/core/validation/TurtleValidator.hpp"
#include "turtle/generated/Turtle/Module.hpp"

namespace turtle {

void install_core_module(TurtleCoreServices& services) {
  turtle::generated::Turtle::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace turtle
