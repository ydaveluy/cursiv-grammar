// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "rfc1035/core/CoreServices.hpp"
#include "rfc1035/core/validation/Rfc1035Validator.hpp"
#include "rfc1035/generated/Rfc1035/Module.hpp"

namespace rfc1035 {

void install_core_module(Rfc1035CoreServices& services) {
  rfc1035::generated::Rfc1035::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace rfc1035
