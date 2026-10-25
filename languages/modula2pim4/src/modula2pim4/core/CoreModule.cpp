// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "modula2pim4/core/CoreServices.hpp"
#include "modula2pim4/core/validation/Modula2pim4Validator.hpp"
#include "modula2pim4/generated/Modula2pim4/Module.hpp"

namespace modula2pim4 {

void install_core_module(Modula2pim4CoreServices& services) {
  modula2pim4::generated::Modula2pim4::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace modula2pim4
