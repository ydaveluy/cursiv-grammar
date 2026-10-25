// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tinyc/core/CoreServices.hpp"
#include "tinyc/core/validation/TinycValidator.hpp"
#include "tinyc/generated/Tinyc/Module.hpp"

namespace tinyc {

void install_core_module(TinycCoreServices& services) {
  tinyc::generated::Tinyc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tinyc
