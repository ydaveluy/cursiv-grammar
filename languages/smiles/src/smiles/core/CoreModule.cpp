// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "smiles/core/CoreServices.hpp"
#include "smiles/core/validation/SmilesValidator.hpp"
#include "smiles/generated/Smiles/Module.hpp"

namespace smiles {

void install_core_module(SmilesCoreServices& services) {
  smiles::generated::Smiles::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace smiles
