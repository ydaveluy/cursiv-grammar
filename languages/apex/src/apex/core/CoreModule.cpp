// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "apex/core/CoreServices.hpp"
#include "apex/core/validation/ApexValidator.hpp"
#include "apex/generated/Apex/Module.hpp"

namespace apex {

void install_core_module(ApexCoreServices& services) {
  apex::generated::Apex::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace apex
