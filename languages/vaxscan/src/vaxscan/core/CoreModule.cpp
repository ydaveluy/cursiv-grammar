// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "vaxscan/core/CoreServices.hpp"
#include "vaxscan/core/validation/VaxscanValidator.hpp"
#include "vaxscan/generated/Vaxscan/Module.hpp"

namespace vaxscan {

void install_core_module(VaxscanCoreServices& services) {
  vaxscan::generated::Vaxscan::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace vaxscan
