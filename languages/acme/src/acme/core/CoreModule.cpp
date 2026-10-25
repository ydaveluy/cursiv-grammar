// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "acme/core/CoreServices.hpp"
#include "acme/core/validation/AcmeValidator.hpp"
#include "acme/generated/Acme/Module.hpp"

namespace acme {

void install_core_module(AcmeCoreServices& services) {
  acme::generated::Acme::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace acme
