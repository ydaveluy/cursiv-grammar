// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "molecule/core/CoreServices.hpp"
#include "molecule/core/validation/MoleculeValidator.hpp"
#include "molecule/generated/Molecule/Module.hpp"

namespace molecule {

void install_core_module(MoleculeCoreServices& services) {
  molecule::generated::Molecule::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace molecule
