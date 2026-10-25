// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tnt/core/CoreServices.hpp"
#include "tnt/core/validation/TntValidator.hpp"
#include "tnt/generated/Tnt/Module.hpp"

namespace tnt {

void install_core_module(TntCoreServices& services) {
  tnt::generated::Tnt::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tnt
