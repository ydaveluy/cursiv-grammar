// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "dice/core/CoreServices.hpp"
#include "dice/core/validation/DiceValidator.hpp"
#include "dice/generated/Dice/Module.hpp"

namespace dice {

void install_core_module(DiceCoreServices& services) {
  dice::generated::Dice::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace dice
