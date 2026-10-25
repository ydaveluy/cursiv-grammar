// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cookie/core/CoreServices.hpp"
#include "cookie/core/validation/CookieValidator.hpp"
#include "cookie/generated/Cookie/Module.hpp"

namespace cookie {

void install_core_module(CookieCoreServices& services) {
  cookie::generated::Cookie::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace cookie
