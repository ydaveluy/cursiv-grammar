// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "microc/core/CoreServices.hpp"
#include "microc/core/validation/MicrocValidator.hpp"
#include "microc/generated/Microc/Module.hpp"

namespace microc {

void install_core_module(MicrocCoreServices& services) {
  microc::generated::Microc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace microc
