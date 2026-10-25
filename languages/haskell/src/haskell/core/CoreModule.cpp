// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "haskell/core/CoreServices.hpp"
#include "haskell/core/validation/HaskellValidator.hpp"
#include "haskell/generated/Haskell/Module.hpp"

namespace haskell {

void install_core_module(HaskellCoreServices& services) {
  haskell::generated::Haskell::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace haskell
