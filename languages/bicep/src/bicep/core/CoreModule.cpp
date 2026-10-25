// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bicep/core/CoreServices.hpp"
#include "bicep/core/validation/BicepValidator.hpp"
#include "bicep/generated/Bicep/Module.hpp"

namespace bicep {

void install_core_module(BicepCoreServices& services) {
  bicep::generated::Bicep::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace bicep
