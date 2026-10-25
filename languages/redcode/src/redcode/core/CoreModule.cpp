// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "redcode/core/CoreServices.hpp"
#include "redcode/core/validation/RedcodeValidator.hpp"
#include "redcode/generated/Redcode/Module.hpp"

namespace redcode {

void install_core_module(RedcodeCoreServices& services) {
  redcode::generated::Redcode::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace redcode
