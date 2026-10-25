// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "wln/core/CoreServices.hpp"
#include "wln/core/validation/WlnValidator.hpp"
#include "wln/generated/Wln/Module.hpp"

namespace wln {

void install_core_module(WlnCoreServices& services) {
  wln::generated::Wln::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace wln
