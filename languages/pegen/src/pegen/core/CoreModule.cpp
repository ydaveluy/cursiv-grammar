// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pegen/core/CoreServices.hpp"
#include "pegen/core/validation/PegenValidator.hpp"
#include "pegen/generated/Pegen/Module.hpp"

namespace pegen {

void install_core_module(PegenCoreServices& services) {
  pegen::generated::Pegen::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pegen
