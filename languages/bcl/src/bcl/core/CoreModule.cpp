// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bcl/core/CoreServices.hpp"
#include "bcl/core/validation/BclValidator.hpp"
#include "bcl/generated/Bcl/Module.hpp"

namespace bcl {

void install_core_module(BclCoreServices& services) {
  bcl::generated::Bcl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace bcl
