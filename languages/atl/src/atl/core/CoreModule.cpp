// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "atl/core/CoreServices.hpp"
#include "atl/core/validation/AtlValidator.hpp"
#include "atl/generated/Atl/Module.hpp"

namespace atl {

void install_core_module(AtlCoreServices& services) {
  atl::generated::Atl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace atl
