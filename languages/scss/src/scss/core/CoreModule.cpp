// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "scss/core/CoreServices.hpp"
#include "scss/core/validation/ScssValidator.hpp"
#include "scss/generated/Scss/Module.hpp"

namespace scss {

void install_core_module(ScssCoreServices& services) {
  scss::generated::Scss::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace scss
