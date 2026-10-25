// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "r/core/CoreServices.hpp"
#include "r/core/validation/RValidator.hpp"
#include "r/generated/R/Module.hpp"

namespace r {

void install_core_module(RCoreServices& services) {
  r::generated::R::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace r
