// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pascal/core/CoreServices.hpp"
#include "pascal/core/validation/PascalValidator.hpp"
#include "pascal/generated/Pascal/Module.hpp"

namespace pascal {

void install_core_module(PascalCoreServices& services) {
  pascal::generated::Pascal::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pascal
