// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "apt/core/CoreServices.hpp"
#include "apt/core/validation/AptValidator.hpp"
#include "apt/generated/Apt/Module.hpp"

namespace apt {

void install_core_module(AptCoreServices& services) {
  apt::generated::Apt::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace apt
