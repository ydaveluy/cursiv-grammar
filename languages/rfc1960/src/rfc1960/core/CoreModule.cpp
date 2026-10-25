// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "rfc1960/core/CoreServices.hpp"
#include "rfc1960/core/validation/Rfc1960Validator.hpp"
#include "rfc1960/generated/Rfc1960/Module.hpp"

namespace rfc1960 {

void install_core_module(Rfc1960CoreServices& services) {
  rfc1960::generated::Rfc1960::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace rfc1960
