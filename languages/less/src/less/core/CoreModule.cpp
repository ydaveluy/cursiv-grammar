// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "less/core/CoreServices.hpp"
#include "less/core/validation/LessValidator.hpp"
#include "less/generated/Less/Module.hpp"

namespace less {

void install_core_module(LessCoreServices& services) {
  less::generated::Less::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace less
