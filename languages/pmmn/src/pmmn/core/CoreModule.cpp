// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pmmn/core/CoreServices.hpp"
#include "pmmn/core/validation/PmmnValidator.hpp"
#include "pmmn/generated/Pmmn/Module.hpp"

namespace pmmn {

void install_core_module(PmmnCoreServices& services) {
  pmmn::generated::Pmmn::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pmmn
