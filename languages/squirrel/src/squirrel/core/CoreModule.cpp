// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "squirrel/core/CoreServices.hpp"
#include "squirrel/core/validation/SquirrelValidator.hpp"
#include "squirrel/generated/Squirrel/Module.hpp"

namespace squirrel {

void install_core_module(SquirrelCoreServices& services) {
  squirrel::generated::Squirrel::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace squirrel
