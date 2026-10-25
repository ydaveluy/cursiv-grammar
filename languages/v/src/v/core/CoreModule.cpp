// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "v/core/CoreServices.hpp"
#include "v/core/validation/VValidator.hpp"
#include "v/generated/V/Module.hpp"

namespace v {

void install_core_module(VCoreServices& services) {
  v::generated::V::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace v
