// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lisa/core/CoreServices.hpp"
#include "lisa/core/validation/LisaValidator.hpp"
#include "lisa/generated/Lisa/Module.hpp"

namespace lisa {

void install_core_module(LisaCoreServices& services) {
  lisa::generated::Lisa::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace lisa
