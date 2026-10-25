// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sdl/core/CoreServices.hpp"
#include "sdl/core/validation/SdlValidator.hpp"
#include "sdl/generated/Sdl/Module.hpp"

namespace sdl {

void install_core_module(SdlCoreServices& services) {
  sdl::generated::Sdl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace sdl
