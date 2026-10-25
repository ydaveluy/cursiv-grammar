// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pf/core/CoreServices.hpp"
#include "pf/core/validation/PfValidator.hpp"
#include "pf/generated/Pf/Module.hpp"

namespace pf {

void install_core_module(PfCoreServices& services) {
  pf::generated::Pf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pf
