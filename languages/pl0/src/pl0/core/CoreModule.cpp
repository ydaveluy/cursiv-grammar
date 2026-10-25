// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pl0/core/CoreServices.hpp"
#include "pl0/core/validation/Pl0Validator.hpp"
#include "pl0/generated/Pl0/Module.hpp"

namespace pl0 {

void install_core_module(Pl0CoreServices& services) {
  pl0::generated::Pl0::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pl0
