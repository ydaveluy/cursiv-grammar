// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cto/core/CoreServices.hpp"
#include "cto/core/validation/CtoValidator.hpp"
#include "cto/generated/Cto/Module.hpp"

namespace cto {

void install_core_module(CtoCoreServices& services) {
  cto::generated::Cto::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace cto
