// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "dart2/core/CoreServices.hpp"
#include "dart2/core/validation/Dart2Validator.hpp"
#include "dart2/generated/Dart2/Module.hpp"

namespace dart2 {

void install_core_module(Dart2CoreServices& services) {
  dart2::generated::Dart2::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace dart2
