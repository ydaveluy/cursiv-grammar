// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lpc/core/CoreServices.hpp"
#include "lpc/core/validation/LpcValidator.hpp"
#include "lpc/generated/Lpc/Module.hpp"

namespace lpc {

void install_core_module(LpcCoreServices& services) {
  lpc::generated::Lpc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace lpc
