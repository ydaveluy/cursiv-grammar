// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "turtledoc/core/CoreServices.hpp"
#include "turtledoc/core/validation/TurtleDocValidator.hpp"
#include "turtledoc/generated/TurtleDoc/Module.hpp"

namespace turtledoc {

void install_core_module(TurtleDocCoreServices& services) {
  turtledoc::generated::TurtleDoc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace turtledoc
