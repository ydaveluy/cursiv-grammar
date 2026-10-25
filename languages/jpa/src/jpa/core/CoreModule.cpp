// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "jpa/core/CoreServices.hpp"
#include "jpa/core/validation/JpaValidator.hpp"
#include "jpa/generated/Jpa/Module.hpp"

namespace jpa {

void install_core_module(JpaCoreServices& services) {
  jpa::generated::Jpa::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace jpa
