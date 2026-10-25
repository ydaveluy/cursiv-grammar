// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tl/core/CoreServices.hpp"
#include "tl/core/validation/TlValidator.hpp"
#include "tl/generated/Tl/Module.hpp"

namespace tl {

void install_core_module(TlCoreServices& services) {
  tl::generated::Tl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tl
