// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "vb6/CoreServices.hpp"
#include "vb6/validation/Vb6Validator.hpp"
#include "vb6/generated/Vb6/Module.hpp"

namespace vb6 {

void install_core_module(Vb6CoreServices& services) {
  vb6::generated::Vb6::installGeneratedModule(services);
  register_validation_checks(services);
}

}  // namespace vb6
