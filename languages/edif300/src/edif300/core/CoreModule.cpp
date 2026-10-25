// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "edif300/core/CoreServices.hpp"
#include "edif300/core/validation/Edif300Validator.hpp"
#include "edif300/generated/Edif300/Module.hpp"

namespace edif300 {

void install_core_module(Edif300CoreServices& services) {
  edif300::generated::Edif300::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace edif300
