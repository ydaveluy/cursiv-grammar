// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "awk/core/CoreServices.hpp"
#include "awk/core/validation/AwkValidator.hpp"
#include "awk/generated/Awk/Module.hpp"

namespace awk {

void install_core_module(AwkCoreServices& services) {
  awk::generated::Awk::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace awk
