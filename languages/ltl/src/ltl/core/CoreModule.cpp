// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "ltl/core/CoreServices.hpp"
#include "ltl/core/validation/LtlValidator.hpp"
#include "ltl/generated/Ltl/Module.hpp"

namespace ltl {

void install_core_module(LtlCoreServices& services) {
  ltl::generated::Ltl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace ltl
