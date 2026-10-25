// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "abb/core/CoreServices.hpp"
#include "abb/core/validation/AbbValidator.hpp"
#include "abb/generated/Abb/Module.hpp"

namespace abb {

void install_core_module(AbbCoreServices& services) {
  abb::generated::Abb::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace abb
