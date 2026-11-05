// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "provn/CoreServices.hpp"
#include "provn/validation/ProvNValidator.hpp"
#include "provn/generated/ProvN/Module.hpp"

namespace provn {

void install_core_module(ProvNCoreServices& services) {
  provn::generated::ProvN::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace provn
