// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "agc/core/CoreServices.hpp"
#include "agc/core/validation/AgcValidator.hpp"
#include "agc/generated/Agc/Module.hpp"

namespace agc {

void install_core_module(AgcCoreServices& services) {
  agc::generated::Agc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace agc
