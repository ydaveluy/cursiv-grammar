// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "spass/core/CoreServices.hpp"
#include "spass/core/validation/SpassValidator.hpp"
#include "spass/generated/Spass/Module.hpp"

namespace spass {

void install_core_module(SpassCoreServices& services) {
  spass::generated::Spass::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace spass
