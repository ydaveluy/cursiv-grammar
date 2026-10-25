// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "algol60/core/CoreServices.hpp"
#include "algol60/core/validation/Algol60Validator.hpp"
#include "algol60/generated/Algol60/Module.hpp"

namespace algol60 {

void install_core_module(Algol60CoreServices& services) {
  algol60::generated::Algol60::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace algol60
