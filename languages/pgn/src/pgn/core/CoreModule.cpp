// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pgn/core/CoreServices.hpp"
#include "pgn/core/validation/PgnValidator.hpp"
#include "pgn/generated/Pgn/Module.hpp"

namespace pgn {

void install_core_module(PgnCoreServices& services) {
  pgn::generated::Pgn::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pgn
