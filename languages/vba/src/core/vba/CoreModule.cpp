// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "vba/CoreServices.hpp"
#include "vba/validation/VbaValidator.hpp"
#include "vba/generated/Vba/Module.hpp"

namespace vba {

void install_core_module(VbaCoreServices& services) {
  vba::generated::Vba::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace vba
