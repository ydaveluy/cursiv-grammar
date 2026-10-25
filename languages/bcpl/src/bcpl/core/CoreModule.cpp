// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bcpl/core/CoreServices.hpp"
#include "bcpl/core/validation/BcplValidator.hpp"
#include "bcpl/generated/Bcpl/Module.hpp"

namespace bcpl {

void install_core_module(BcplCoreServices& services) {
  bcpl::generated::Bcpl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace bcpl
