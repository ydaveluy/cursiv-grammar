// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "morsecode/core/CoreServices.hpp"
#include "morsecode/core/validation/MorsecodeValidator.hpp"
#include "morsecode/generated/Morsecode/Module.hpp"

namespace morsecode {

void install_core_module(MorsecodeCoreServices& services) {
  morsecode::generated::Morsecode::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace morsecode
