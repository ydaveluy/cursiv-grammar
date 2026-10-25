// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "fen/core/CoreServices.hpp"
#include "fen/core/validation/FenValidator.hpp"
#include "fen/generated/Fen/Module.hpp"

namespace fen {

void install_core_module(FenCoreServices& services) {
  fen::generated::Fen::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace fen
