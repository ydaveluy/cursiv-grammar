// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bison/core/CoreServices.hpp"
#include "bison/core/validation/BisonValidator.hpp"
#include "bison/generated/Bison/Module.hpp"

namespace bison {

void install_core_module(BisonCoreServices& services) {
  bison::generated::Bison::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace bison
