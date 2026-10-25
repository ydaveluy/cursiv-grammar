// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "css3/core/CoreServices.hpp"
#include "css3/core/validation/Css3Validator.hpp"
#include "css3/generated/Css3/Module.hpp"

namespace css3 {

void install_core_module(Css3CoreServices& services) {
  css3::generated::Css3::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace css3
