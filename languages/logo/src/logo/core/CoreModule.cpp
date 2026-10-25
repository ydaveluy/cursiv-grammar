// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "logo/core/CoreServices.hpp"
#include "logo/core/validation/LogoValidator.hpp"
#include "logo/generated/Logo/Module.hpp"

namespace logo {

void install_core_module(LogoCoreServices& services) {
  logo::generated::Logo::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace logo
