// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "itn/core/CoreServices.hpp"
#include "itn/core/validation/ItnValidator.hpp"
#include "itn/generated/Itn/Module.hpp"

namespace itn {

void install_core_module(ItnCoreServices& services) {
  itn::generated::Itn::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace itn
