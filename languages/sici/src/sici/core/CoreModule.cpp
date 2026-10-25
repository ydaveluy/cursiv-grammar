// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sici/core/CoreServices.hpp"
#include "sici/core/validation/SiciValidator.hpp"
#include "sici/generated/Sici/Module.hpp"

namespace sici {

void install_core_module(SiciCoreServices& services) {
  sici::generated::Sici::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace sici
