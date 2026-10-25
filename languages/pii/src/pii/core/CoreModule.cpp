// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pii/core/CoreServices.hpp"
#include "pii/core/validation/PiiValidator.hpp"
#include "pii/generated/Pii/Module.hpp"

namespace pii {

void install_core_module(PiiCoreServices& services) {
  pii::generated::Pii::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pii
