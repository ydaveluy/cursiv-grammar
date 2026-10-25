// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "edn/core/CoreServices.hpp"
#include "edn/core/validation/EdnValidator.hpp"
#include "edn/generated/Edn/Module.hpp"

namespace edn {

void install_core_module(EdnCoreServices& services) {
  edn::generated::Edn::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace edn
