// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "szf/core/CoreServices.hpp"
#include "szf/core/validation/SzfValidator.hpp"
#include "szf/generated/Szf/Module.hpp"

namespace szf {

void install_core_module(SzfCoreServices& services) {
  szf::generated::Szf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace szf
