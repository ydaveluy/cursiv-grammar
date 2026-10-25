// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "muparser/core/CoreServices.hpp"
#include "muparser/core/validation/MuparserValidator.hpp"
#include "muparser/generated/Muparser/Module.hpp"

namespace muparser {

void install_core_module(MuparserCoreServices& services) {
  muparser::generated::Muparser::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace muparser
