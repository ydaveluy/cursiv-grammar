// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "postalcode/core/CoreServices.hpp"
#include "postalcode/core/validation/PostalcodeValidator.hpp"
#include "postalcode/generated/Postalcode/Module.hpp"

namespace postalcode {

void install_core_module(PostalcodeCoreServices& services) {
  postalcode::generated::Postalcode::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace postalcode
