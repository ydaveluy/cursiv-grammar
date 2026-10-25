// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pdn/core/CoreServices.hpp"
#include "pdn/core/validation/PdnValidator.hpp"
#include "pdn/generated/Pdn/Module.hpp"

namespace pdn {

void install_core_module(PdnCoreServices& services) {
  pdn::generated::Pdn::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pdn
