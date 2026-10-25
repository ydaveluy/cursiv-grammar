// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "blueprint/core/CoreServices.hpp"
#include "blueprint/core/validation/BlueprintValidator.hpp"
#include "blueprint/generated/Blueprint/Module.hpp"

namespace blueprint {

void install_core_module(BlueprintCoreServices& services) {
  blueprint::generated::Blueprint::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace blueprint
