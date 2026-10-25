// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "fol/core/CoreServices.hpp"
#include "fol/core/validation/FolValidator.hpp"
#include "fol/generated/Fol/Module.hpp"

namespace fol {

void install_core_module(FolCoreServices& services) {
  fol::generated::Fol::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace fol
