// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pddl/core/CoreServices.hpp"
#include "pddl/core/validation/PddlValidator.hpp"
#include "pddl/generated/Pddl/Module.hpp"

namespace pddl {

void install_core_module(PddlCoreServices& services) {
  pddl::generated::Pddl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pddl
