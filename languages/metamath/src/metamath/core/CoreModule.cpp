// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "metamath/core/CoreServices.hpp"
#include "metamath/core/validation/MetamathValidator.hpp"
#include "metamath/generated/Metamath/Module.hpp"

namespace metamath {

void install_core_module(MetamathCoreServices& services) {
  metamath::generated::Metamath::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace metamath
