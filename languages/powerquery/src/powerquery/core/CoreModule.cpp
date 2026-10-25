// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "powerquery/core/CoreServices.hpp"
#include "powerquery/core/validation/PowerqueryValidator.hpp"
#include "powerquery/generated/Powerquery/Module.hpp"

namespace powerquery {

void install_core_module(PowerqueryCoreServices& services) {
  powerquery::generated::Powerquery::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace powerquery
