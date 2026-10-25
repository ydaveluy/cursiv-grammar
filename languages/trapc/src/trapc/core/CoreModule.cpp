// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "trapc/core/CoreServices.hpp"
#include "trapc/core/validation/TrapcValidator.hpp"
#include "trapc/generated/Trapc/Module.hpp"

namespace trapc {

void install_core_module(TrapcCoreServices& services) {
  trapc::generated::Trapc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace trapc
