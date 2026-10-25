// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "odin/core/CoreServices.hpp"
#include "odin/core/validation/OdinValidator.hpp"
#include "odin/generated/Odin/Module.hpp"

namespace odin {

void install_core_module(OdinCoreServices& services) {
  odin::generated::Odin::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace odin
