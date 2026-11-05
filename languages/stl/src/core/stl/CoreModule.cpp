// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "stl/CoreServices.hpp"
#include "stl/validation/StlValidator.hpp"
#include "stl/generated/Stl/Module.hpp"

namespace stl {

void install_core_module(StlCoreServices& services) {
  stl::generated::Stl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace stl
