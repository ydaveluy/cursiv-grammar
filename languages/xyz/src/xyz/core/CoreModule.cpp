// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "xyz/core/CoreServices.hpp"
#include "xyz/core/validation/XyzValidator.hpp"
#include "xyz/generated/Xyz/Module.hpp"

namespace xyz {

void install_core_module(XyzCoreServices& services) {
  xyz::generated::Xyz::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace xyz
