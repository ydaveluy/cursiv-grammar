// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "newick/core/CoreServices.hpp"
#include "newick/core/validation/NewickValidator.hpp"
#include "newick/generated/Newick/Module.hpp"

namespace newick {

void install_core_module(NewickCoreServices& services) {
  newick::generated::Newick::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace newick
