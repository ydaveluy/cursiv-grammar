// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "gvpr/core/CoreServices.hpp"
#include "gvpr/core/validation/GvprValidator.hpp"
#include "gvpr/generated/Gvpr/Module.hpp"

namespace gvpr {

void install_core_module(GvprCoreServices& services) {
  gvpr::generated::Gvpr::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace gvpr
