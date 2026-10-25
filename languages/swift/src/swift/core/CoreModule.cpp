// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "swift/core/CoreServices.hpp"
#include "swift/core/validation/SwiftValidator.hpp"
#include "swift/generated/Swift/Module.hpp"

namespace swift {

void install_core_module(SwiftCoreServices& services) {
  swift::generated::Swift::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace swift
