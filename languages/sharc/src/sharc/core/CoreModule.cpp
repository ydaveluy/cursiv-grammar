// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sharc/core/CoreServices.hpp"
#include "sharc/core/validation/SharcValidator.hpp"
#include "sharc/generated/Sharc/Module.hpp"

namespace sharc {

void install_core_module(SharcCoreServices& services) {
  sharc::generated::Sharc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace sharc
