// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "vba/core/CoreServices.hpp"
#include "vba/core/validation/VbaValidator.hpp"
#include "vba/generated/Vba/Module.hpp"

namespace vba {

void install_core_module(VbaCoreServices& services) {
  vba::generated::Vba::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace vba
