// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "zig/core/CoreServices.hpp"
#include "zig/core/validation/ZigValidator.hpp"
#include "zig/generated/Zig/Module.hpp"

namespace zig {

void install_core_module(ZigCoreServices& services) {
  zig::generated::Zig::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace zig
