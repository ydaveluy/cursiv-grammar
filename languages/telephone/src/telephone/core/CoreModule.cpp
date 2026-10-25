// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "telephone/core/CoreServices.hpp"
#include "telephone/core/validation/TelephoneValidator.hpp"
#include "telephone/generated/Telephone/Module.hpp"

namespace telephone {

void install_core_module(TelephoneCoreServices& services) {
  telephone::generated::Telephone::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace telephone
