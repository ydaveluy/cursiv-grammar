// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "swiftfin/core/CoreServices.hpp"
#include "swiftfin/core/validation/SwiftFinValidator.hpp"
#include "swiftfin/generated/SwiftFin/Module.hpp"

namespace swiftfin {

void install_core_module(SwiftFinCoreServices& services) {
  swiftfin::generated::SwiftFin::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace swiftfin
