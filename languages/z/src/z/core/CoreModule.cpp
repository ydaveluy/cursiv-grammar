// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "z/core/CoreServices.hpp"
#include "z/core/validation/ZValidator.hpp"
#include "z/generated/Z/Module.hpp"

namespace z {

void install_core_module(ZCoreServices& services) {
  z::generated::Z::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace z
