// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "star/core/CoreServices.hpp"
#include "star/core/validation/StarValidator.hpp"
#include "star/generated/Star/Module.hpp"

namespace star {

void install_core_module(StarCoreServices& services) {
  star::generated::Star::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace star
