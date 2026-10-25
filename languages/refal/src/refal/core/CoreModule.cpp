// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "refal/core/CoreServices.hpp"
#include "refal/core/validation/RefalValidator.hpp"
#include "refal/generated/Refal/Module.hpp"

namespace refal {

void install_core_module(RefalCoreServices& services) {
  refal::generated::Refal::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace refal
