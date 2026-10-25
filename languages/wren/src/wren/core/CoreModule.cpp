// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "wren/core/CoreServices.hpp"
#include "wren/core/validation/WrenValidator.hpp"
#include "wren/generated/Wren/Module.hpp"

namespace wren {

void install_core_module(WrenCoreServices& services) {
  wren::generated::Wren::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace wren
