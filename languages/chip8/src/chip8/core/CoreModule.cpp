// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "chip8/core/CoreServices.hpp"
#include "chip8/core/validation/Chip8Validator.hpp"
#include "chip8/generated/Chip8/Module.hpp"

namespace chip8 {

void install_core_module(Chip8CoreServices& services) {
  chip8::generated::Chip8::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace chip8
