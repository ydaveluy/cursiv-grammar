// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "snobol/core/CoreServices.hpp"
#include "snobol/core/validation/SnobolValidator.hpp"
#include "snobol/generated/Snobol/Module.hpp"

namespace snobol {

void install_core_module(SnobolCoreServices& services) {
  snobol::generated::Snobol::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace snobol
