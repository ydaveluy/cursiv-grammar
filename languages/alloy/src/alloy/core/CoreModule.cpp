// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "alloy/core/CoreServices.hpp"
#include "alloy/core/validation/AlloyValidator.hpp"
#include "alloy/generated/Alloy/Module.hpp"

namespace alloy {

void install_core_module(AlloyCoreServices& services) {
  alloy::generated::Alloy::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace alloy
