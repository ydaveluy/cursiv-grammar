// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "callable/core/CoreServices.hpp"
#include "callable/core/validation/CallableValidator.hpp"
#include "callable/generated/Callable/Module.hpp"

namespace callable {

void install_core_module(CallableCoreServices& services) {
  callable::generated::Callable::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace callable
