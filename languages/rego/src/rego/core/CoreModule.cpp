// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "rego/core/CoreServices.hpp"
#include "rego/core/validation/RegoValidator.hpp"
#include "rego/generated/Rego/Module.hpp"

namespace rego {

void install_core_module(RegoCoreServices& services) {
  rego::generated::Rego::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace rego
