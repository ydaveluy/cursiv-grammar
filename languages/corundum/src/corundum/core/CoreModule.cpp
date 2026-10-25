// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "corundum/core/CoreServices.hpp"
#include "corundum/core/validation/CorundumValidator.hpp"
#include "corundum/generated/Corundum/Module.hpp"

namespace corundum {

void install_core_module(CorundumCoreServices& services) {
  corundum::generated::Corundum::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace corundum
