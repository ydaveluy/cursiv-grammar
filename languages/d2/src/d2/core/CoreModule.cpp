// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "d2/core/CoreServices.hpp"
#include "d2/core/validation/D2Validator.hpp"
#include "d2/generated/D2/Module.hpp"

namespace d2 {

void install_core_module(D2CoreServices& services) {
  d2::generated::D2::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace d2
