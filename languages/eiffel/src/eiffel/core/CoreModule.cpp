// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "eiffel/core/CoreServices.hpp"
#include "eiffel/core/validation/EiffelValidator.hpp"
#include "eiffel/generated/Eiffel/Module.hpp"

namespace eiffel {

void install_core_module(EiffelCoreServices& services) {
  eiffel::generated::Eiffel::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace eiffel
