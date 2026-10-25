// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cmake/core/CoreServices.hpp"
#include "cmake/core/validation/CmakeValidator.hpp"
#include "cmake/generated/Cmake/Module.hpp"

namespace cmake {

void install_core_module(CmakeCoreServices& services) {
  cmake::generated::Cmake::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace cmake
