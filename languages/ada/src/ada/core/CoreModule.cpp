// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "ada/core/CoreServices.hpp"
#include "ada/core/validation/AdaValidator.hpp"
#include "ada/generated/Ada/Module.hpp"

namespace ada {

void install_core_module(AdaCoreServices& services) {
  ada::generated::Ada::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace ada
