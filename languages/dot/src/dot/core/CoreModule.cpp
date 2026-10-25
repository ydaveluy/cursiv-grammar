// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "dot/core/CoreServices.hpp"
#include "dot/core/validation/DotValidator.hpp"
#include "dot/generated/Dot/Module.hpp"

namespace dot {

void install_core_module(DotCoreServices& services) {
  dot::generated::Dot::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace dot
