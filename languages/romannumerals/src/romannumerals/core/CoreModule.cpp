// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "romannumerals/core/CoreServices.hpp"
#include "romannumerals/core/validation/RomannumeralsValidator.hpp"
#include "romannumerals/generated/Romannumerals/Module.hpp"

namespace romannumerals {

void install_core_module(RomannumeralsCoreServices& services) {
  romannumerals::generated::Romannumerals::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace romannumerals
