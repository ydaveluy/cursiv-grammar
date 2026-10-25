// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "basic/core/CoreServices.hpp"
#include "basic/core/validation/BasicValidator.hpp"
#include "basic/generated/Basic/Module.hpp"

namespace basic {

void install_core_module(BasicCoreServices& services) {
  basic::generated::Basic::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace basic
