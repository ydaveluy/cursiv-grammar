// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "rfc3080/core/CoreServices.hpp"
#include "rfc3080/core/validation/Rfc3080Validator.hpp"
#include "rfc3080/generated/Rfc3080/Module.hpp"

namespace rfc3080 {

void install_core_module(Rfc3080CoreServices& services) {
  rfc3080::generated::Rfc3080::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace rfc3080
