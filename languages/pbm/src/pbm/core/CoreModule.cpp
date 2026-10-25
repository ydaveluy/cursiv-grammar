// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pbm/core/CoreServices.hpp"
#include "pbm/core/validation/PbmValidator.hpp"
#include "pbm/generated/Pbm/Module.hpp"

namespace pbm {

void install_core_module(PbmCoreServices& services) {
  pbm::generated::Pbm::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pbm
