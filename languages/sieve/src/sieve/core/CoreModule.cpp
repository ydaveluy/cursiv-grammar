// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sieve/core/CoreServices.hpp"
#include "sieve/core/validation/SieveValidator.hpp"
#include "sieve/generated/Sieve/Module.hpp"

namespace sieve {

void install_core_module(SieveCoreServices& services) {
  sieve::generated::Sieve::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace sieve
