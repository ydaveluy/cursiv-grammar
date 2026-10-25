// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "rfc822/core/CoreServices.hpp"
#include "rfc822/core/validation/Rfc822Validator.hpp"
#include "rfc822/generated/Rfc822/Module.hpp"

namespace rfc822 {

void install_core_module(Rfc822CoreServices& services) {
  rfc822::generated::Rfc822::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace rfc822
