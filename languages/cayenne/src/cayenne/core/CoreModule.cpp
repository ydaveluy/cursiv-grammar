// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cayenne/core/CoreServices.hpp"
#include "cayenne/core/validation/CayenneValidator.hpp"
#include "cayenne/generated/Cayenne/Module.hpp"

namespace cayenne {

void install_core_module(CayenneCoreServices& services) {
  cayenne::generated::Cayenne::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace cayenne
