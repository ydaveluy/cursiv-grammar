// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "rexx/core/CoreServices.hpp"
#include "rexx/core/validation/RexxValidator.hpp"
#include "rexx/generated/Rexx/Module.hpp"

namespace rexx {

void install_core_module(RexxCoreServices& services) {
  rexx::generated::Rexx::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace rexx
