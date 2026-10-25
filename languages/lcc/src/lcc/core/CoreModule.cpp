// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lcc/core/CoreServices.hpp"
#include "lcc/core/validation/LccValidator.hpp"
#include "lcc/generated/Lcc/Module.hpp"

namespace lcc {

void install_core_module(LccCoreServices& services) {
  lcc::generated::Lcc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace lcc
