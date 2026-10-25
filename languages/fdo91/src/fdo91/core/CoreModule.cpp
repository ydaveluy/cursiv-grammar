// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "fdo91/core/CoreServices.hpp"
#include "fdo91/core/validation/Fdo91Validator.hpp"
#include "fdo91/generated/Fdo91/Module.hpp"

namespace fdo91 {

void install_core_module(Fdo91CoreServices& services) {
  fdo91::generated::Fdo91::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace fdo91
