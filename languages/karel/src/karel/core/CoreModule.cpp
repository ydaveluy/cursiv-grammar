// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "karel/core/CoreServices.hpp"
#include "karel/core/validation/KarelValidator.hpp"
#include "karel/generated/Karel/Module.hpp"

namespace karel {

void install_core_module(KarelCoreServices& services) {
  karel::generated::Karel::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace karel
