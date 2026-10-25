// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "infosapient/core/CoreServices.hpp"
#include "infosapient/core/validation/InfosapientValidator.hpp"
#include "infosapient/generated/Infosapient/Module.hpp"

namespace infosapient {

void install_core_module(InfosapientCoreServices& services) {
  infosapient::generated::Infosapient::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace infosapient
