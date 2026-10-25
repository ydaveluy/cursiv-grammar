// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "dif/core/CoreServices.hpp"
#include "dif/core/validation/DifValidator.hpp"
#include "dif/generated/Dif/Module.hpp"

namespace dif {

void install_core_module(DifCoreServices& services) {
  dif::generated::Dif::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace dif
