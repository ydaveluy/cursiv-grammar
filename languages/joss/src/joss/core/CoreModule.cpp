// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "joss/core/CoreServices.hpp"
#include "joss/core/validation/JossValidator.hpp"
#include "joss/generated/Joss/Module.hpp"

namespace joss {

void install_core_module(JossCoreServices& services) {
  joss::generated::Joss::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace joss
