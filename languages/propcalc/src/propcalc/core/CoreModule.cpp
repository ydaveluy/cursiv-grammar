// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "propcalc/core/CoreServices.hpp"
#include "propcalc/core/validation/PropcalcValidator.hpp"
#include "propcalc/generated/Propcalc/Module.hpp"

namespace propcalc {

void install_core_module(PropcalcCoreServices& services) {
  propcalc::generated::Propcalc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace propcalc
