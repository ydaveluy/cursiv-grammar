// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "kirikiritjs/core/CoreServices.hpp"
#include "kirikiritjs/core/validation/KirikiriTjsValidator.hpp"
#include "kirikiritjs/generated/KirikiriTjs/Module.hpp"

namespace kirikiritjs {

void install_core_module(KirikiriTjsCoreServices& services) {
  kirikiritjs::generated::KirikiriTjs::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace kirikiritjs
