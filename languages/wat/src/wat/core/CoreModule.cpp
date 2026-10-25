// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "wat/core/CoreServices.hpp"
#include "wat/core/validation/WatValidator.hpp"
#include "wat/generated/Wat/Module.hpp"

namespace wat {

void install_core_module(WatCoreServices& services) {
  wat::generated::Wat::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace wat
